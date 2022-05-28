#include <iostream>
#include <cstdint>


#define ELF_NIDENT	16

// program header-ы такого типа нужно загрузить в
// память при загрузке приложения
#define PT_LOAD		1

// структура заголовка ELF файла
struct elf_hdr {
	std::uint8_t e_ident[ELF_NIDENT];
	std::uint16_t e_type;
	std::uint16_t e_machine;
	std::uint32_t e_version;
	std::uint64_t e_entry;
	std::uint64_t e_phoff;
	std::uint64_t e_shoff;
	std::uint32_t e_flags;
	std::uint16_t e_ehsize;
	std::uint16_t e_phentsize;
	std::uint16_t e_phnum;
	std::uint16_t e_shentsize;
	std::uint16_t e_shnum;
	std::uint16_t e_shstrndx;
} __attribute__((packed));

// структура записи в таблице program header-ов
struct elf_phdr {
	std::uint32_t p_type;
	std::uint32_t p_flags;
	std::uint64_t p_offset;
	std::uint64_t p_vaddr;
	std::uint64_t p_paddr;
	std::uint64_t p_filesz;
	std::uint64_t p_memsz;
	std::uint64_t p_align;
} __attribute__((packed));



std::size_t space(const char *name) {
  // Ваш код здесь, name - имя ELF файла, с которым вы работаете
  // вернуть нужно количество байт, необходимых, чтобы загрузить
  // приложение в память

  FILE* elf_fp = fopen( name, "rb" );
  if ( !elf_fp )
    return -1;

  struct elf_hdr elf_file_hdr = {0};
  // read file header
  if ( fread( &elf_file_hdr, sizeof( struct elf_hdr ), 1, elf_fp ) != 1 ) {
    fclose( elf_fp );
    return -1;
  }

  // check if elf file
  if ( elf_file_hdr.e_ident[ 0 ] != 0x7f     // Magic number
    || elf_file_hdr.e_ident[ 1 ] != 0x45     // E
    || elf_file_hdr.e_ident[ 2 ] != 0x4c     // L
    || elf_file_hdr.e_ident[ 3 ] != 0x46 ) { // F
    fclose( elf_fp );
    return -1;
  }

  // jump to the start of program headers
  if ( fseek( elf_fp, elf_file_hdr.e_phoff, SEEK_SET ) != 0 ) {
    fclose( elf_fp );
    return -1;
  }

  struct elf_phdr prg_hdrs[ elf_file_hdr.e_phnum ];
  // read program headers
  if ( fread( prg_hdrs, elf_file_hdr.e_phentsize, elf_file_hdr.e_phnum, elf_fp ) != elf_file_hdr.e_phnum ) {
    fclose( elf_fp );
    return -1;
  }
  fclose( elf_fp );

  std::size_t memsize = 0;
  for ( std::size_t i = 0; i < elf_file_hdr.e_phnum; ++i )
    if ( prg_hdrs[ i ].p_type == PT_LOAD )
      memsize += prg_hdrs[ i ].p_memsz;

	return memsize;
}

int main( int argc, char** argv ) {
	if ( argc != 2 ) {
		std::cout << "usage: readelf <path-to-elf>" << std::endl;
		return -1;
	}
	
	std::cout << space( argv[1] ) << std::endl;
	return 0;
}