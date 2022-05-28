#include <iostream>
#include <cstdlib>

struct border_tag {
    std::size_t size;
    bool is_free;
};

static void* head = NULL;
static std::size_t heap_sz = 0;

#define border_tag_sz sizeof( struct border_tag )
#define tail ( void* ) ( ( std::uintptr_t ) head + heap_sz)

static struct border_tag* left_tag( void* memblk ) {
    return ( struct border_tag* ) ( ( std::uintptr_t ) memblk - border_tag_sz );
}

static struct border_tag* right_tag( void* memblk ) {
    struct border_tag* lt = left_tag( memblk );
    return ( struct border_tag* ) ( ( std::uintptr_t ) memblk + lt->size );
}

static void* memblk_addr_lt( void* left_tag ) {
    return ( void* ) ( ( std::uintptr_t ) left_tag + border_tag_sz );
}

static void* memblk_addr_rt( void* right_tag ) {
    struct border_tag* rt = ( struct border_tag* ) right_tag;
    return ( void* ) ( ( std::uintptr_t ) right_tag - rt->size );
}

static std::uintptr_t blk_size( std::uintptr_t capacity ) {
    return capacity - 2 * border_tag_sz;
}

static std::uintptr_t blk_cap( std::uintptr_t size ) {
    return size + 2 * border_tag_sz;
}

static void init_with_tags( void* buffer, struct border_tag tag ) {
    struct border_tag* lt = ( struct border_tag* ) buffer;
    *lt = tag;
    
    void* memblk = memblk_addr_lt( lt );
    struct border_tag* rt = right_tag( memblk );
    *rt = tag;
}

static void* next_blk( void* memblk ) {
    struct border_tag* lt = left_tag( memblk );
    return memblk_addr_lt( ( void* ) ( ( std::uintptr_t ) lt + blk_cap( lt->size ) ) );
}

// Эта функция будет вызвана перед тем как вызывать myalloc и myfree
// используйте ее чтобы инициализировать ваш аллокатор перед началом
// работы.
//
// buf - указатель на участок логической памяти, который ваш аллокатор
//       должен распределять, все возвращаемые указатели должны быть
//       либо равны NULL, либо быть из этого участка памяти
// size - размер участка памяти, на который указывает buf
void mysetup( void* buf, std::size_t size ) {
    struct border_tag init_tag = { blk_size( size ), true };
    init_with_tags( buf, init_tag );
    head = buf;
    heap_sz = size;
}

static bool is_good_blk( std::size_t size, struct border_tag btag ) {
    return btag.is_free && btag.size >= size;
}

static struct border_tag* look_for_good_blk( void* head, std::size_t size ) {
    void* iter = memblk_addr_lt( head );
    struct border_tag* iter_tag = left_tag( iter );
    while ( ( iter < tail ) && !is_good_blk( size, *iter_tag ) ) {
        iter = next_blk( iter );
        iter_tag = left_tag( iter );
    }

    if ( ( ( std::uintptr_t ) iter >= ( std::uintptr_t ) tail ) || !is_good_blk( size, *iter_tag ) ) return NULL;
    
    return iter_tag;
}

// Функция аллокации
void* myalloc( std::size_t size ) {
    struct border_tag* lt_maybe_blk = look_for_good_blk( head, size );
    if ( lt_maybe_blk == NULL ) return NULL;

    const std::size_t old_sz = lt_maybe_blk->size;
    const std::uintptr_t new_cap = blk_cap( old_sz ) - blk_cap( size );
    if ( new_cap <= 2 * border_tag_sz )
        size = old_sz;

    init_with_tags( lt_maybe_blk, ( struct border_tag ) { size, false } );
    if ( new_cap > 2 * border_tag_sz ) {
        void* rest = next_blk( memblk_addr_lt( lt_maybe_blk ) );
        struct border_tag rest_tag = { blk_size( new_cap ), true };
        init_with_tags( left_tag( rest ), rest_tag );
    }
    return memblk_addr_lt( lt_maybe_blk );
}

// Функция освобождения
void myfree( void* p ) {
    // std::cout << "freeing[ " << p << " ]" << std::endl;
    if ( p == NULL )
        return;

    struct border_tag* lt = left_tag( p );
    if ( lt > head ) {
        struct border_tag* prev_rt = left_tag( lt );
        if ( prev_rt->is_free ) {
            // std::cout << "  unite with prev" << std::endl;
            // unite with previous block
            void* prev_memblk = memblk_addr_rt( prev_rt );
            const std::size_t new_size = blk_size( blk_cap( prev_rt->size ) + blk_cap( lt->size ) );
            init_with_tags( left_tag( prev_memblk ), ( struct border_tag ) { new_size, false } );
            p = prev_memblk;
        }
    }

    lt = left_tag( p );
    struct border_tag* rt = right_tag( p );
    struct border_tag* next_lt = ( struct border_tag* ) memblk_addr_lt( rt );
    if ( ( std::uintptr_t ) next_lt < ( std::uintptr_t ) tail )
        if ( next_lt->is_free ) {
            // std::cout << "  unite with next" << std::endl;
            // unite with next block
            const std::size_t new_size = blk_size( blk_cap( rt->size ) + blk_cap( next_lt->size ) );
            init_with_tags( lt, ( struct border_tag ) { new_size, false } );
        }

    init_with_tags( lt, ( struct border_tag ) { lt->size, true } );
}

static void memdump( ) {
    std::cout << "------------------------------------------" << std::endl;
    void* iter = memblk_addr_lt( head );
    struct border_tag* rt = right_tag( iter );
    while ( ( std::uintptr_t ) ( ( std::uintptr_t ) rt + border_tag_sz ) <= ( std::uintptr_t ) tail ) {
        struct border_tag* lt = left_tag( iter );
        std::cout << "address: " << iter << std::endl;
        std::cout << "size: " << lt->size << std::endl;
        std::cout << "free: " << lt->is_free << std::endl;
        std::cout << "tags addresses:" << std::endl;
        std::cout << "  left:  " << lt << std::endl;
        std::cout << "  right: " << rt << std::endl;
        std::cout << std::endl;
        iter = next_blk( iter );
        rt = right_tag( iter );
    }
    std::cout << std::endl;
}

int main( int argc, char** argv ) {
    std::size_t heap_size = 351;
    void* heap = malloc( heap_size * sizeof( char ) );

    mysetup( heap, heap_size );
    memdump( );

    void* p1 = myalloc( 80 );
    void* p2 = myalloc( 61 );
    void* p3 = myalloc( 39 );
    memdump( );

    void* p4 = myalloc( 44 );
    memdump( );

    myfree( p4 );
    myfree( p3 );
    memdump( );

    myfree( p1 );
    memdump( );

    myfree( p2 );
    memdump( );

    free( heap );
    return 0;
} 