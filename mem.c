/* On inclut l'interface publique */
#include "mem.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
*/
struct allocator_header {
        size_t memory_size;
        struct fb* first;
	mem_fit_function_t *fit;
};

/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static void* memory_addr;

static inline void *get_system_memory_addr() {
	return memory_addr;
}

static inline struct allocator_header *get_header() {
	struct allocator_header *h;
	h = get_system_memory_addr();
	return h;
}

static inline size_t get_system_memory_size() {
	return get_header()->memory_size;
}


struct fb {
	size_t size;
	struct fb* next;
	/* ... */
};

void mem_init(void* mem, size_t taille) {
    memory_addr = mem;
    *(size_t*)memory_addr = taille;
	/* On vérifie qu'on a bien enregistré les infos et qu'on
	 * sera capable de les récupérer par la suite
	 */
	assert(mem == get_system_memory_addr());
	assert(taille == get_system_memory_size());
	
	struct fb* freeblock = (struct fb*)(get_system_memory_addr()+sizeof(struct allocator_header));
	freeblock->size = taille-sizeof(struct allocator_header);
	freeblock->next = get_system_memory_addr()+get_system_memory_size();

	get_header()->first = freeblock;
	
	mem_fit(&mem_fit_first);
}
/*while < SIZE*/
void mem_show(void (*print)(void *, size_t, int)) {
	struct fb* ptr_current_fb = get_header()->first;
	void* ptr_current_zone = get_system_memory_addr()+sizeof(struct allocator_header);
	void *ptr_end_of_memory = get_system_memory_addr();
	ptr_end_of_memory = ptr_end_of_memory+(get_system_memory_size()-sizeof(struct allocator_header));

	size_t block_size;
	while (ptr_current_zone < ptr_end_of_memory) {
		block_size = *(size_t*)ptr_current_zone;
		if (ptr_current_zone < (void*)ptr_current_fb) {	// Zone occupée
			print(ptr_current_zone, *(size_t*)ptr_current_zone, 0);
		} else {						// Zone libre
			print(ptr_current_zone, ptr_current_fb->size, 1);
			ptr_current_fb = ptr_current_fb->next;
		}
		ptr_current_zone += block_size;
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void *mem_alloc(size_t taille) {
	__attribute__((unused)) /* juste pour que gcc compile ce squelette avec -Werror */

	size_t taille_reelle = taille+sizeof(size_t);
	taille_reelle += (8 - taille_reelle % 8);
	struct fb *fb=get_header()->fit(get_header()->first, taille_reelle);
	if (fb == NULL) {
		return NULL;
	}
	// On sauvegarde les informations de la zone libre que l'on va modifier
	size_t taille_prec = fb->size;
	struct fb *next_prec = fb->next;
	
	// On trouve la zone libre précédant la zone libre que l'on va modifier
	struct fb *fb_prec = get_header()->first;

	if (fb_prec != fb) {	// Pas besoin de parcours si la mémoire n'est qu'une seule zone libre
		while (fb_prec->next != fb) {
			fb_prec = fb_prec->next;
		}
	}

	// size_t * pour la zone allouée

	void *fb_alias = fb;
	size_t *zone_allouee = fb_alias;
	if (taille_prec-taille_reelle >= sizeof(struct fb)) {
		*zone_allouee = taille_reelle;
		fb = fb_alias+taille_reelle;
		fb->next = next_prec;
		fb->size = taille_prec-taille_reelle;
		fb_prec->next = fb;
	} else {
		taille_reelle = taille_prec;
		*zone_allouee = taille_reelle;
		fb_prec->next = next_prec;
	}
	if (fb_alias == get_header()->first) {
		get_header()->first = fb_prec->next;
	}
	return zone_allouee;
}


void mem_free(void* mem) {
    struct fb* ptr_current_fb = get_header()->first;
    struct fb* previous_fb=get_header()->first; //init pr eviter seg fault si liberation 1ere ZL
    int is_allocated_before=0;
    void* ptr_current_zone = get_system_memory_addr()+sizeof(struct allocator_header);
    void* addr_to_free = mem;
    size_t block_size;

    while (ptr_current_zone < addr_to_free ) {
        block_size = *(size_t*)ptr_current_zone;

        if (ptr_current_zone == (void*)ptr_current_fb) { //si on est sur ZL, on va chercher la prochaine
            previous_fb = ptr_current_fb;
            ptr_current_fb = ptr_current_fb->next;
            is_allocated_before=0;

        } else { 
            is_allocated_before=1;
        }
		
        ptr_current_zone += block_size;
        ptr_current_zone= (size_t*)ptr_current_zone;
    }

    if ((void*)ptr_current_zone != addr_to_free || ptr_current_zone== (size_t*)ptr_current_fb){ //Erreur, il n'y a pas de bloc à l'adresse mem
        return;                                                                    //OU Erreur, le bloc à libérer est deja libre
    }

    block_size = *(size_t*)ptr_current_zone;
    int is_allocated_after=(ptr_current_zone+block_size != (size_t*)ptr_current_fb);

    struct fb* new_fb;
    new_fb = (struct fb *)ptr_current_zone;
    new_fb->size=*(size_t*)ptr_current_zone;
    new_fb->next=NULL;

	if (ptr_current_fb==previous_fb) { //si on est a la tete
        get_header()->first = new_fb;

        if (!is_allocated_after) {
            new_fb->size+=ptr_current_fb->size;
            new_fb->next=ptr_current_fb->next;
        } else {
            new_fb->next=ptr_current_fb;
        }

    } else if (!is_allocated_before && !is_allocated_after) {    
        previous_fb->size += block_size+ptr_current_fb->size;
        previous_fb->next=ptr_current_fb->next;

     } else if (!is_allocated_before) {
        previous_fb->size += block_size;
        previous_fb->next=ptr_current_fb;

     } else if (!is_allocated_after) {
         previous_fb->next=new_fb;
         new_fb->size+=ptr_current_fb->size;
         new_fb->next=ptr_current_fb->next;

     } else {
         previous_fb->next=new_fb;
         new_fb->next=ptr_current_fb;
     }
}


struct fb* mem_fit_first(struct fb *list, size_t size) {
    struct fb* current = list;
    while(current != NULL) {
        if(current->size >= size) {
            return current;
		}
        current = current->next;
    }
    return NULL;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	return 0;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	struct fb* current = list;
	struct fb* tmp = current;
	while (current->size < size){
		if (current == NULL) {
			return NULL;
		}
		current = current->next;
	}
	while (current != NULL) {
		if (current->size >= size && current->size < tmp->size) {
			tmp = current;
		}
		current = current->next;
	}
	return tmp;
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
    struct fb* current = list;
    struct fb* tmp = current;
    while(current != NULL) {
        if(current->size >= tmp->size) {
            tmp = current;
		}
        current = current->next;
    }
    if (tmp->size < size) {
		return NULL;
	} else { 
        return tmp;
	}
}
