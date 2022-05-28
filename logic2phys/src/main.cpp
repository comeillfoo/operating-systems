#include <iostream>
#include <cstdint>
#include <map>

// 47     39 38    30 29       21 20   12 11     0
//   +------+--------+-----------+-------+--------+
//   | PML4 | DirPtr | Directory | Table | Offset |
//   +------+--------+-----------+-------+--------+

struct laddr_ptrs {
    std::size_t pml4;
    std::size_t directory_ptr;
    std::size_t directory;
    std::size_t table;
    std::size_t offset;
};

#define PTRS_MASK 0x1ff
#define PAGE_FAULT -1

static struct laddr_ptrs parse_laddr( std::size_t laddr ) {
    const std::size_t pml4 = ( laddr >> 39 ) & PTRS_MASK;
    const std::size_t directory_ptr = ( laddr >> 30 ) & PTRS_MASK;
    const std::size_t directory = ( laddr >> 21 ) & PTRS_MASK;
    const std::size_t table = ( laddr >> 12 ) & PTRS_MASK;
    const std::size_t offset = laddr & 0xfff; // 1111 1111 1111
    return ( struct laddr_ptrs ) { pml4, directory_ptr, directory, table, offset };
}

static std::size_t logic2phys( std::size_t, std::size_t, std::map<std::size_t, std::size_t>& );

int main( int argc, char** argv ) {
    std::size_t m = 0, q = 0, cr3 = 0;
    std::cin >> m >> q >> cr3;

    std::map<std::size_t, std::size_t> memory;

    for ( std::size_t i = 0; i < m; ++i ) {
        std::size_t paddr = 0, value = 0;
        std::cin >> paddr >> value;
        memory[ paddr ] = value;
    }

    for ( std::size_t i = 0; i < q; ++i ) {
        std::size_t laddr = 0;
        std::cin >> laddr;
        const std::size_t paddr = logic2phys( cr3, laddr, memory );
        
        if ( paddr == PAGE_FAULT )
            std::cout << "fault" << std::endl;
        else std::cout << paddr << std::endl;
    }
    return 0;
}

static std::size_t laddr2table( std::size_t laddr, std::size_t offset, std::map<std::size_t, std::size_t>& memory ) {
    const std::size_t paddr = laddr + ( offset << 3 ); // 'cause the size of page is 4KB
    if ( memory.count( paddr ) < 1 )
        return PAGE_FAULT;

    const std::size_t table_entry = memory[ paddr ];
    // check if P is reset
    if ( table_entry & 1 == 0 )
        return PAGE_FAULT;
    
    return table_entry & 0x000ffffffffff000;
}


static std::size_t logic2phys( std::size_t cr3, std::size_t laddr, std::map<std::size_t, std::size_t>& memory ) {
    const struct laddr_ptrs la = parse_laddr( laddr );
    
    const std::size_t directory_ptr_table_addr = laddr2table( cr3, la.pml4, memory );
    if ( directory_ptr_table_addr == PAGE_FAULT )
        return PAGE_FAULT;

    const std::size_t directory_table_addr = laddr2table( directory_ptr_table_addr, la.directory_ptr, memory );
    if ( directory_table_addr == PAGE_FAULT )
        return PAGE_FAULT;

    const std::size_t table_table_addr = laddr2table( directory_table_addr, la.directory, memory );
    if ( table_table_addr == PAGE_FAULT )
        return PAGE_FAULT;

    const std::size_t physical_table_addr = laddr2table( table_table_addr, la.table, memory );
    if ( physical_table_addr == PAGE_FAULT )
        return 0;

    const std::size_t paddr = physical_table_addr + la.offset;
    return paddr;
}