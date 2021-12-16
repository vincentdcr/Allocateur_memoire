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
	
	struct fb tmp = {taille, NULL};
	struct fb* freeblock = &tmp;

	get_header()->first = freeblock;
	
	mem_fit(&mem_fit_first);
}
/*while < SIZE*/
void mem_show(void (*print)(void *, size_t, int)) {
	struct fb* ptr_tmp = get_header()->first;
	size_t* pos = get_system_memory_addr();
	size_t block_size;
	while ((size_t)pos < (size_t)(get_system_memory_size()+memory_addr)) {
		block_size = *pos;
		if (pos < (size_t*)ptr_tmp) {
			print(pos, *pos, 1);
		} else {
			print(pos, ptr_tmp->size, 0);
			ptr_tmp = ptr_tmp->next;
		}
		pos += block_size;
		pos= (size_t*)pos;
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void *mem_alloc(size_t taille) {
	/* ... */
	__attribute__((unused)) /* juste pour que gcc compile ce squelette avec -Werror */
	struct fb *fb=get_header()->fit(/*...*/NULL, /*...*/0);
	/* ... */
	return NULL;
}


void mem_free(void* mem) {
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
