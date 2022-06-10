#include <stdlib.h>

/**
 * Эти две функции вы должны использовать для аллокации
 * и освобождения памяти в этом задании. Считайте, что
 * внутри они используют buddy аллокатор с размером
 * страницы равным 4096 байтам.
 **/
struct memblk {
    struct memblk* next;
};

struct slab_hdr {
    struct memblk* blocks;
    size_t nr_free;
    struct slab_hdr* prev;
    struct slab_hdr* next;
};

/**
 * Аллоцирует участок размером 4096 * 2^order байт,
 * выровненный на границу 4096 * 2^order байт. order
 * должен быть в интервале [0; 10] (обе границы
 * включительно), т. е. вы не можете аллоцировать больше
 * 4Mb за раз.
 **/
void* alloc_slab(int order);
/**
 * Освобождает участок ранее аллоцированный с помощью
 * функции alloc_slab.
 **/
void free_slab(void *slab);

#define BUDDY_PAGE_SIZE 4096 // byte
#define MINIMAL_ELEMENTS_IN_ONE_SLAB 32 // elements

static size_t slab_capacity( int order ) { return BUDDY_PAGE_SIZE * ( 1UL << order ); }
static void*  slab_p( void* obj_p, int order ) {
    const long unsigned nr_obj_p = (long unsigned) obj_p;
    return (void*) ( nr_obj_p & ~(slab_capacity( order ) - 1) );
}

static void* memblk_p( void* obj_p ) {
    const long unsigned nr_obj_p = (long unsigned) obj_p;
    return (void*) ( nr_obj_p - sizeof(struct memblk) );
}

static void* mem_p( struct memblk* blk ) {
    const long unsigned nr_blk_p = (long unsigned) blk;
    return (void*) ( nr_blk_p + sizeof(struct memblk) );
}

static size_t object_capacity( size_t object_size ) {
    return sizeof( struct memblk ) + object_size;
}

static size_t slab_size( size_t object_size ) {
    return sizeof( struct slab_hdr ) + MINIMAL_ELEMENTS_IN_ONE_SLAB * object_capacity( object_size );
}

/**
 * Эта структура представляет аллокатор, вы можете менять
 * ее как вам удобно. Приведенные в ней поля и комментарии
 * просто дают общую идею того, что вам может понадобится
 * сохранить в этой структуре.
 **/
struct cache {
    /* список пустых SLAB-ов для поддержки cache_shrink */
    struct slab_hdr* empty_list;
    /* список частично занятых SLAB-ов */
    struct slab_hdr* non_empty_list;
    /* список заполненых SLAB-ов */
    struct slab_hdr* filled_list;

    size_t object_size; /* размер аллоцируемого объекта */
    int slab_order; /* используемый размер SLAB-а */
    size_t slab_objects; /* количество объектов в одном SLAB-е */ 
};


/**
 * Функция инициализации будет вызвана перед тем, как
 * использовать это кеширующий аллокатор для аллокации.
 * Параметры:
 *  - cache - структура, которую вы должны инициализировать
 *  - object_size - размер объектов, которые должен
 *    аллоцировать этот кеширующий аллокатор 
 **/
void cache_setup(struct cache *cache, size_t object_size)
{
    /* Реализуйте эту функцию. */
    int order = 0;
    size_t meta_size = sizeof( struct slab_hdr );

    cache->filled_list = NULL;
    cache->non_empty_list = NULL;
    cache->empty_list = NULL;
    cache->object_size = object_size;

    const size_t minimal_required_memory = slab_size( object_size );

    /* getting the required slab order */
    while ( slab_capacity( order ) < minimal_required_memory ) ++order;
    cache->slab_order = order;

    const size_t nr_objects = (slab_capacity( order ) / minimal_required_memory - 1);
    const size_t nr_extra_bytes = nr_objects * minimal_required_memory + slab_capacity( order ) % minimal_required_memory;
    const size_t nr_required_objects = MINIMAL_ELEMENTS_IN_ONE_SLAB + nr_extra_bytes / object_capacity( object_size );
    cache->slab_objects = nr_required_objects;
}

static void free_slab_list( struct slab_hdr* list ) {
    struct slab_hdr* iter = list;
    while ( iter ) {
        struct slab_hdr* tmp = iter->next;
        free_slab( iter );
        iter = tmp;
    }
}

/**
 * Функция освобождения будет вызвана когда работа с
 * аллокатором будет закончена. Она должна освободить
 * всю память занятую аллокатором. Проверяющая система
 * будет считать ошибкой, если не вся память будет
 * освбождена.
 **/
void cache_release(struct cache *cache)
{
    /* Реализуйте эту функцию. */
    free_slab_list( cache->filled_list );
    cache->filled_list = NULL;
    free_slab_list( cache->non_empty_list );
    cache->non_empty_list = NULL;
    free_slab_list( cache->empty_list );
    cache->empty_list = NULL;
}

static void slab_list_add_slab_front( struct cache* cache, struct slab_hdr** list_p, struct slab_hdr* slab ) {
    // if slab is the head of any list, need to reassign new head for the list
    if ( slab == cache->non_empty_list )
        cache->non_empty_list = slab->next;
    else if ( slab == cache->empty_list )
        cache->empty_list = slab->next;
    else if ( slab == cache->filled_list )
        cache->filled_list = slab->next;

    /* removing slab from the old list */ 
    if ( slab->prev )
        slab->prev->next = slab->next;
    if ( slab->next )
        slab->next->prev = slab->prev;

    slab->prev = NULL;
    slab->next = *list_p;
    
    if ( *list_p )
        (*list_p)->prev = slab;
    *list_p = slab;
}

static struct memblk* slab_list_extract_front( struct slab_hdr* list ) {
    struct memblk* front = list->blocks;
    list->blocks = front->next;
    front->next = NULL; // because otherwise it may broke the other lists
    return front;
}

static void slab_list_init( struct slab_hdr* list, size_t object_size, size_t nr_objects ) {
#define MEMBLK_OF_SLAB(x) ((struct memblk*) (((long unsigned) (x)) + sizeof(struct slab_hdr)))
    struct memblk* first = MEMBLK_OF_SLAB(list);
#undef MEMBLK_OF_SLAB
    const size_t shift = sizeof(struct memblk) + object_size;
    struct memblk* iter = first;
    for ( size_t i = 0; i < nr_objects; ++i ) {
        struct memblk* next = (nr_objects == i + 1)? NULL : (struct memblk*) (((long unsigned) iter) + shift);
        iter->next = next;
        iter = next;
    }

    list->blocks = first;
    list->nr_free = nr_objects;
}

static struct slab_hdr* slab_create( int order ) {
    struct slab_hdr* slab = (struct slab_hdr*) alloc_slab( order );
    slab->blocks = NULL;
    slab->nr_free = 0;
    slab->next = NULL;
    slab->prev = NULL;
    return slab;
}

/**
 * Функция аллокации памяти из кеширующего аллокатора.
 * Должна возвращать указатель на участок памяти размера
 * как минимум object_size байт (см cache_setup).
 * Гарантируется, что cache указывает на корректный
 * инициализированный аллокатор.
 **/
void *cache_alloc(struct cache *cache)
{
    /* Реализуйте эту функцию. */
    struct memblk* blk;
    if ( cache->non_empty_list ) { /* if list is not empty */
        blk = slab_list_extract_front( cache->non_empty_list );
        cache->non_empty_list->nr_free--;
        if ( cache->non_empty_list->nr_free == 0 ) /* if no more free objects in list then move to the filled_list */
            slab_list_add_slab_front( cache, &(cache->filled_list), cache->non_empty_list );
    } else if ( cache->empty_list ) {
        blk = slab_list_extract_front( cache->empty_list );
        cache->empty_list->nr_free--;
        slab_list_add_slab_front( cache, &(cache->non_empty_list), cache->empty_list );
    } else {
        struct slab_hdr* slab = slab_create( cache->slab_order );
        slab_list_init( slab, cache->object_size, cache->slab_objects );
        blk = slab_list_extract_front( slab );
        slab->nr_free--;
        cache->non_empty_list = slab;
    }
    return mem_p( blk );
}

static void slab_list_add_blk_front( struct slab_hdr* list, struct memblk* blk ) {
    blk->next = list->blocks;
    list->blocks = blk;
}

/**
 * Функция освобождения памяти назад в кеширующий аллокатор.
 * Гарантируется, что ptr - указатель ранее возвращенный из
 * cache_alloc.
 **/
void cache_free(struct cache *cache, void *ptr)
{
    /* Реализуйте эту функцию. */
    struct slab_hdr* ptr_slab = (struct slab_hdr*) slab_p( ptr, cache->slab_order );
    struct memblk* ptr_memblk = (struct memblk*) memblk_p( ptr );

    slab_list_add_blk_front( ptr_slab, ptr_memblk );
    ptr_slab->nr_free++;

    if ( ptr_slab->nr_free == 1 )/* we don't need to put again non empty slab in the same slab list*/
        slab_list_add_slab_front( cache, &(cache->non_empty_list), ptr_slab ); // move slab into non empty slab list
    else if ( ptr_slab->nr_free == cache->slab_objects )
        slab_list_add_slab_front( cache, &(cache->empty_list), ptr_slab ); // move slab into empty slab list
}


/**
 * Функция должна освободить все SLAB, которые не содержат
 * занятых объектов. Если SLAB не использовался для аллокации
 * объектов (например, если вы выделяли с помощью alloc_slab
 * память для внутренних нужд вашего алгоритма), то освбождать
 * его не обязательно.
 **/
void cache_shrink(struct cache *cache)
{
    /* Реализуйте эту функцию. */
    free_slab_list( cache->empty_list );
    cache->empty_list = NULL;
}