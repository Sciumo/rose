/* Copyright 2008 Lawrence Livermore National Security, LLC */
#ifndef Exec_ExecNE_h
#define Exec_ExecNE_h

#include "ExecDOS.h"

namespace Exec {
namespace NE {

/* Forwards */
class NEFileHeader;
class NESectionTable;
class NENameTable;
class NEStringTable;
class NEModuleTable;
class NEEntryTable;
class NERelocTable;
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExtendedDOSHeader -- extra components of the DOS header when used in an NE file
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ExtendedDOSHeader_disk {
    uint16_t    e_res1[14];             /* reserved */
    uint32_t    e_lfanew;               /* file offset for NE header */
} __attribute__((packed));

class ExtendedDOSHeader : public ExecSection {
  public:
    ExtendedDOSHeader(ExecFile *f, addr_t offset)
        : ExecSection(f, offset, sizeof(ExtendedDOSHeader_disk))
        {ctor(f, offset);}
    virtual ~ExtendedDOSHeader() {};
    void *encode(ExtendedDOSHeader_disk*);
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);

    /* These are the native-format versions of the same members described in the ExtendedDOSHeader_disk struct. */
    unsigned e_res1[14];
    addr_t e_lfanew;

  private:
    void ctor(ExecFile *f, addr_t offset);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE File Header
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* File format of an NE File Header. All fields are little endian.
 *
 * NOTES
 * 
 * e_sssp: The value specified in SS is an index (1-origin) into the segment table. If SS addresses the automatic data segment
 *         and SP is zero then SP is set to the address obtained by adding the size of the automatic data segment to the size
 *         of the stack. */
struct NEFileHeader_disk {
    unsigned char e_magic[2];           /* 0x00 magic number "NE" */
    unsigned char e_linker_major;       /* 0x02 linker major version number */
    unsigned char e_linker_minor;       /* 0x03 linker minor version number */
    uint16_t    e_entrytab_rfo;         /* 0x04 entry table offset relative to start of header */
    uint16_t    e_entrytab_size;        /* 0x06 size of entry table in bytes */
    uint32_t    e_checksum;             /* 0x08 32-bit CRC of entire file (this word is taken a zero during the calculation) */
    uint16_t    e_flags1;               /* 0x0c file-level bit flags (see HeaderFlags1) */
    uint16_t    e_autodata_sn;          /* 0x0e auto data section number if (flags & 0x3)==0; else zero */
    uint16_t    e_bss_size;             /* 0x10 num bytes added to data segment for BSS */
    uint16_t    e_stack_size;           /* 0x12 num bytes added to data segment for stack (zero of SS!=DS registers) */
    uint32_t    e_csip;                 /* 0x14 section number:offset of CS:IP */
    uint32_t    e_sssp;                 /* 0x18 section number:offset of SS:SP (see note 1 above) */
    uint16_t    e_nsections;            /* 0x1c number of entries in the section table */
    uint16_t    e_nmodrefs;             /* 0x1e number of entries in the module reference table */
    uint16_t    e_nnonresnames;         /* 0x20 number of entries in the non-resident name table */
    uint16_t    e_sectab_rfo;           /* 0x22 offset of section table relative to start of header */
    uint16_t    e_rsrctab_rfo;          /* 0x24 offset of resource table relative to start of header */
    uint16_t    e_resnametab_rfo;       /* 0x26 offset of resident name table relative to start of header */
    uint16_t    e_modreftab_rfo;        /* 0x28 offset of module reference table relative to start of header */
    uint16_t    e_importnametab_rfo;    /* 0x2a offset of imported names table relative to start of header */
    uint32_t    e_nonresnametab_offset; /* 0x2c file offset of non-resident name table */
    uint16_t    e_nmovable_entries;     /* 0x30 number of movable entries in Entry Table */
    uint16_t    e_sector_align;         /* 0x32 sector alignment shift count (log2 of segment sector size) */
    uint16_t    e_nresources;           /* 0x34 number of resource entries */
    unsigned char e_exetype;            /* 0x36 executable type (2==windows) */
    unsigned char e_flags2;             /* 0x37 additional flags (see HeaderFlags2) */
    uint16_t    e_fastload_sector;      /* 0x38 sector offset to fast-load area (only for Windows) */
    uint16_t    e_fastload_nsectors;    /* 0x3a size of fast-load area in sectors (only for Windows) */
    uint16_t    e_res1;                 /* 0x3c reserved */
    uint16_t    e_winvers;              /* 0x3e expected version number for Windows (only for Windows) */
} __attribute__((packed));              /* 0x40 */

/* Bit flags for the NE header 'e_flags' member.
 *
 * If HF_LIBRARY (bit 15) is set then the CS:IP registers point to an initialization procedure called with the value in the AX
 * register equal to the module handle. The initialization procedure must execute a far return to the caller. The resulting
 * value in AX is a status indicator (non-zero for success, zero for failure). */
enum HeaderFlags1 {
    HF1_RESERVED         = 0x57f4,      /* Reserved bits */
    HF1_NO_DATA          = 0x0000,      /* (flags&0x03==0) => an exe not containing a data segment */
    HF1_SINGLE_DATA      = 0x0001,      /* Executable contains one data segment; set if file is a DLL */
    HF1_MULTIPLE_DATA    = 0x0002,      /* Exe with multiple data segments; set if a windows application */
    HF1_LOADER_SEGMENT   = 0x0800,      /* First segment contains code that loads the application */
    HF1_FATAL_ERRORS     = 0x2000,      /* Errors detected at link time; module will not load */
    HF1_LIBRARY          = 0x8000,      /* Module is a library */
};

/* Bit flags for the NE header 'e_flags2' member. */
enum HeaderFlags2 {
    HF2_RESERVED         = 0xf1,        /* Reserved bits */
    HF2_PROTECTED_MODE   = 0x02,        /* Windows 2.x application that runs in 3.x protected mode */
    HF2_PFONTS           = 0x04,        /* Windows 2.x application that supports proportional fonts */
    HF2_FASTLOAD         = 0x08         /* Executable contains a fast-load area */
};

class NEFileHeader : public ExecHeader {
  public:
    NEFileHeader(ExecFile *f, addr_t offset)
        : ExecHeader(f, offset, sizeof(NEFileHeader_disk)),
        dos2_header(NULL), section_table(NULL), resname_table(NULL), nonresname_table(NULL), module_table(NULL), entry_table(NULL)
        {ctor(f, offset);}
    virtual ~NEFileHeader() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
    virtual const char *format_name() {return "NE";}

    /* Accessors for protected/private data members */
    ExtendedDOSHeader *get_dos2_header() {return dos2_header;}
    void set_dos2_header(ExtendedDOSHeader *h) {dos2_header=h;}
    NESectionTable *get_section_table() {return section_table;}
    void set_section_table(NESectionTable *ot) {section_table=ot;}
    NENameTable *get_resname_table() {return resname_table;}
    void set_resname_table(NENameTable *ot) {resname_table=ot;}
    NENameTable *get_nonresname_table() {return nonresname_table;}
    void set_nonresname_table(NENameTable *ot) {nonresname_table=ot;}
    NEModuleTable *get_module_table() {return module_table;}
    void set_module_table(NEModuleTable *ot) {module_table=ot;}
    NEEntryTable *get_entry_table() {return entry_table;}
    void set_entry_table(NEEntryTable *ot) {entry_table=ot;}
    
    /* These are the native-format versions of the same members described in the NEFileHeader_disk format struct. */
    unsigned    e_linker_major, e_linker_minor, e_checksum, e_flags1, e_autodata_sn, e_bss_size, e_stack_size;
    unsigned    e_csip, e_sssp, e_nsections, e_nmodrefs, e_nnonresnames, e_nmovable_entries, e_sector_align;
    unsigned    e_nresources, e_exetype, e_flags2, e_res1, e_winvers;
    addr_t      e_entrytab_rfo, e_entrytab_size, e_sectab_rfo, e_rsrctab_rfo, e_resnametab_rfo, e_modreftab_rfo;
    addr_t      e_importnametab_rfo, e_nonresnametab_offset, e_fastload_sector, e_fastload_nsectors;

  private:
    void ctor(ExecFile *f, addr_t offset);
    void *encode(NEFileHeader_disk*);
    ExtendedDOSHeader *dos2_header;
    NESectionTable *section_table;
    NENameTable *resname_table;
    NENameTable *nonresname_table;
    NEModuleTable *module_table;
    NEEntryTable *entry_table;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE Section (Segment) Table
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
/* File format of a section table entry. All fields are little endian. */
struct NESectionTableEntry_disk {
    uint16_t    sector;                 /* 0x00 File offset (sector size defined in hdr); zero means no file data */
    uint16_t    physical_size;          /* 0x02 Length of segment in file; zero means 64k if sector is non-zero */
    uint16_t    flags;                  /* 0x04 Segment bit flags */
    uint16_t    virtual_size;           /* 0x06 Total size of segment when mapped to memory; zero means 64k */
} __attribute__((packed));              /* 0x08 */

enum NESectionFlags {
    SF_RESERVED         = 0x0e08,       /* these bits are reserved */

    SF_TYPE_MASK        = 0x0007,       /* segment-type field */
    SF_CODE             = 0x0000,       /* code-segment type */
    SF_DATA             = 0x0001,       /* data-segment (otherwise segment is code) */
    SF_ALLOC            = 0x0002,       /* loader allocates memory */
    SF_LOAD             = 0x0004,       /* load the segment */

    SF_MOVABLE          = 0x0010,       /* segment is not fixed */
    SF_PURE             = 0x0020,       /* segment is pure, or sharable; otherwise impure or non-sharable */
    SF_PRELOAD          = 0x0040,       /* segment will be preloaded; read-only if this is a data segment */
    SF_NOT_WRITABLE     = 0x0080,       /* code segment is execute only; data segment is read-only */
    SF_RELOCINFO        = 0x0100,       /* segment has relocation records */
    SF_DISCARDABLE      = 0x1000,       /* discardable */
    SF_DISCARD          = 0xf000        /* discard priority */
};

class NESectionTableEntry {
  public:
    NESectionTableEntry(const NESectionTableEntry_disk *disk)
        {ctor(disk);}
    virtual ~NESectionTableEntry() {};
    void *encode(NESectionTableEntry_disk*);
    virtual void dump(FILE *f, const char *prefix, ssize_t idx) {dump(f, prefix, idx, NULL);}
    void dump(FILE*, const char *prefix, ssize_t idx, NEFileHeader *fhdr);
    
    /* These are the native-format versions of the same members described in the NESectionTableEntry_disk struct. */
    unsigned    flags, sector;
    addr_t      physical_size, virtual_size;

  private:
    void ctor(const NESectionTableEntry_disk*);
};

/* Non-synthesized NE sections (i.e., present in the section table) */
class NESection : public ExecSection {
  public:
    NESection(ExecFile *ef, addr_t offset, addr_t size)
        : ExecSection(ef, offset, size),
        st_entry(NULL), reloc_table(NULL)
        {}
    virtual ~NESection() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);

    /* Accessors for protected/private data */
    NESectionTableEntry *get_st_entry() {return st_entry;}
    void set_st_entry(NESectionTableEntry *e) {st_entry=e;}
    NERelocTable *get_reloc_table() {return reloc_table;}
    void set_reloc_table(NERelocTable *t) {reloc_table=t;}

  private:
    NESectionTableEntry *st_entry;
    NERelocTable *reloc_table;
};

/* The table entries are stored in the sections themselves. */
class NESectionTable : public ExecSection {
  public:
    NESectionTable(NEFileHeader *fhdr)
        : ExecSection(fhdr->get_file(), fhdr->end_offset(), fhdr->e_nsections*sizeof(NESectionTableEntry_disk))
        {ctor(fhdr);}
    virtual ~NESectionTable() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
  private:
    void ctor(NEFileHeader*);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resident and Non-Resident Name Tables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* This table contains a module name followed by the list of exported function names. Each name is associated with an "ordinal"
 * which serves as an index into the Entry Table. The ordinal for the first string (module name) is meaningless and should be
 * zero. In the non-resident name table the first entry is a module description and the functions are not always resident in
 * system memory (they are discardable). */
class NENameTable : public ExecSection {
  public:
    NENameTable(NEFileHeader *fhdr, addr_t offset)
        : ExecSection(fhdr->get_file(), offset, 0)
        {ctor(fhdr);}
    virtual ~NENameTable() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
    std::vector<std::string> get_names_by_ordinal(unsigned ordinal);
  private:
    void ctor(NEFileHeader*);
    std::vector<std::string> names; /*first name is module name; remainder are symbols within the module*/
    std::vector<unsigned> ordinals; /*first entry is ignored but present in file*/
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE Module Reference Table
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* The module-reference table comes after the resident name table. */
class NEModuleTable : public ExecSection {
  public:
    NEModuleTable(NEFileHeader *fhdr, addr_t offset, addr_t size, NEStringTable *strtab)
        : ExecSection(fhdr->get_file(), offset, size), strtab(strtab)
        {ctor(fhdr);}
    virtual ~NEModuleTable() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
  private:
    void ctor(NEFileHeader *fhdr);
    NEStringTable *strtab;
    std::vector<addr_t> name_offsets;
    std::vector<std::string> names;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE String Table
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* A section whose contents consist of strings. Each string is introduced by a one-byte length and followed by that number
 * of ASCII characters. Strings are not NUL-terminated. */
class NEStringTable : public ExecSection {
  public:
    NEStringTable(NEFileHeader *fhdr, addr_t offset, addr_t length)
        : ExecSection(fhdr->get_file(), offset, length)
        {ctor(fhdr);}
    virtual ~NEStringTable() {}
    //virtual void unparse(FILE*); /*not needed; use parent's implementation*/
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
    std::string get_string(addr_t offset);
  private:
    void ctor(NEFileHeader*);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE Entry Table
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum NEEntryFlags {
    EF_RESERVED         = 0xfc, /* Reserved bits */
    EF_EXPORTED         = 0x01, /* Exported */
    EF_GLOBAL           = 0x02  /* Uses a global (shared) data section */
};

struct NEEntryPoint {
    NEEntryPoint()
        : flags((NEEntryFlags)0), int3f(0), section_idx(0), section_offset(0)
        {}
    NEEntryPoint(NEEntryFlags flags, unsigned int3f, unsigned s_idx, unsigned s_off)
        : flags(flags), int3f(int3f), section_idx(s_idx), section_offset(s_off)
        {}
    void dump(FILE*, const char *prefix, ssize_t idx) const;
    NEEntryFlags flags;         /* bit flags */
    unsigned int3f;             /* always 0x3fxx */
    unsigned section_idx;       /* zero indicates unused entry */
    unsigned section_offset;    /* byte offset into section */
};

class NEEntryTable : public ExecSection {
  public:
    NEEntryTable(NEFileHeader *fhdr, addr_t offset, addr_t size)
        : ExecSection(fhdr->get_file(), offset, size)
        {ctor(fhdr);}
    virtual ~NEEntryTable() {}
    void populate_entries();
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
  private:
    void ctor(NEFileHeader*);
    std::vector<size_t> bundle_sizes;
    std::vector<NEEntryPoint> entries;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NE Relocation Table
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* NERelocEntry_disk -- variable size with multiple levels of 'union'. It's easier to just parse it in NERelocEntry::ctor()
 * than defining it here as a struct. */

enum NERelocSrcType {
    RF_SRCTYPE_8OFF     = 0,            /* Byte offset */
    RF_SRCTYPE_WORDSEG  = 2,            /* Word segment, 16-bit selector */
    RF_SRCTYPE_16PTR    = 3,            /* 16-bit far pointer */
    RF_SRCTYPE_16OFF    = 5,            /* 16-bit offset */
    RF_SRCTYPE_32PTR    = 6,            /* 32-bit far pointer */
    RF_SRCTYPE_32OFF    = 7,            /* 32-bit offset */
    RF_SRCTYPE_NEARCALL = 8,            /* near call or jump, WORD/DWROD based on section attribute */
    RF_SRCTYPE_48PTR    = 11,           /* 48-bit pointer */
    RF_SRCTYPE_32OFF_b  = 13            /* 32-bit offset (not sure how this differs from case 7) */
};

enum NERelocTgtType {
    RF_TGTTYPE_IREF     = 0,            /* Internal reference */
    RF_TGTTYPE_IORD     = 1,            /* Imported (extern) ordinal */
    RF_TGTTYPE_INAME    = 2,            /* Imported (extern) name */
    RF_TGTTYPE_OSFIXUP  = 3             /* Operating system fixup */ 
};

enum NERelocModifiers {
    RF_MODIFIER_SINGLE  = 1,
    RF_MODIFIER_MULTI   = 3
};

enum NERelocFlags {
    RF_ADDITIVE         = 0x01,         /* add target to source rather than replace source with target */
    RF_RESERVED         = 0x02,         /* reserved bits */
    RF_2EXTRA           = 0x04,         /* relocation info has size with new two bytes at end */
    RF_32ADD            = 0x08,         /* addition with 32-bits rather than 16 */
    RF_16SECTION        = 0x10,         /* 16-bit object number & module name rather than 8-bit */
    RF_8ORDINAL         = 0x20,         /* Ordinal is 8-bits rather than 16 */
};

class NERelocEntry {
  public:
    NERelocEntry(ExecSection *relocs, addr_t at, addr_t *rec_size)
        {ctor(relocs, at, rec_size);}
    void ctor(ExecSection*, addr_t at, addr_t *rec_size);
    addr_t unparse(FILE*, ExecSection*, addr_t spos);
    void dump(FILE*, const char *prefix, ssize_t idx);
  public:
    NERelocSrcType      src_type;       /* low nibble of first byte of relocation record */
    NERelocModifiers    modifier;       /* high nibble of first byte */
    NERelocTgtType      tgt_type;       /* low two bits of second byte */
    NERelocFlags        flags;          /* high six bits of second byte */
    addr_t              src_offset;

#if 0
 // DQ (8/7/2008): This is required in the member function: NERelocEntry::ctor(ExecSection *relocs, addr_t at, addr_t *rec_size)
 // this is an anonymous union defining specific variables of type "struct" to occupy the all start at the same memory location.
 // It is not clear how to represent this in the IR???  We need to discuss this!!!
    union {
        struct { /*tgt_type==0x00: internal reference*/
            unsigned    sect_idx;       /* section index (1-origin) */
            unsigned    res1;           /* reserved */
            addr_t      tgt_offset;
        } iref;
        struct { /*tgt_type==0x01: imported ordinal*/
            unsigned    modref;         /* 1-based index into import module table */
            unsigned    ordinal;
            addr_t      addend;         /* value to add (only present for flags & RF_2EXTRA) */
        } iord;
        struct { /*tgt_type==0x02: imported name*/
            unsigned    modref;         /* 1-based index into import module table */
            unsigned    nm_off;         /* offset into import procedure names */
            addr_t      addend;         /* value to add (only present for flags & RF_2EXTRA) */
        } iname;
        struct { /*tgt_type==0x03: operating system fixup*/
            unsigned    type;
            unsigned    res3;
        } osfixup;
    };
#else
  // DQ (8/7/2008): At only (I hope) the risk of using more memory that required, break the union so that we can better support 
  // this in ROSETTA. One solution might be to implement a common base class of unsigned, unsigned, addr_t; and then use member 
  // functions to access the data in the base class.
     struct iref_type
        { /*tgt_type==0x00: internal reference*/
          unsigned    sect_idx;       /* section index (1-origin) */
          unsigned    res1;           /* reserved */
          addr_t      tgt_offset;

       // Added to support RTI support in ROSE
          friend std::ostream & operator<< ( std::ostream & os, const iref_type & x );

          iref_type();
        };

iref_type iref;

     struct iord_type { /*tgt_type==0x01: imported ordinal*/
          unsigned    modref;         /* 1-based index into import module table */
          unsigned    ordinal;
          addr_t      addend;         /* value to add (only present for flags & RF_2EXTRA) */

       // Added to support RTI support in ROSE
          friend std::ostream & operator<< ( std::ostream & os, const iord_type & x );

          iord_type();
        };

iord_type iord;

     struct iname_type { /*tgt_type==0x02: imported name*/
          unsigned    modref;         /* 1-based index into import module table */
          unsigned    nm_off;         /* offset into import procedure names */
          addr_t      addend;         /* value to add (only present for flags & RF_2EXTRA) */

       // Added to support RTI support in ROSE
          friend std::ostream & operator<< ( std::ostream & os, const iname_type & x );

          iname_type();
        };

iname_type iname;

     struct osfixup_type { /*tgt_type==0x03: operating system fixup*/
          unsigned    type;
          unsigned    res3;

       // Added to support RTI support in ROSE
          friend std::ostream & operator<< ( std::ostream & os, const osfixup_type & x );

          osfixup_type();
        };

osfixup_type osfixup;

#endif

};

class NERelocTable : public ExecSection {
  public:
    NERelocTable(NEFileHeader *fhdr, addr_t offset)
        : ExecSection(fhdr->get_file(), offset, 0)
        {ctor(fhdr);}
    virtual ~NERelocTable() {}
    virtual void unparse(FILE*);
    virtual void dump(FILE*, const char *prefix, ssize_t idx);
  private:
    void ctor(NEFileHeader*);
    std::vector<NERelocEntry> entries;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Functions */
bool is_NE(ExecFile*);
void parseBinaryFormat(ExecFile*, SgAsmFile* asmFile);
NEFileHeader *parse(ExecFile*);

}; //namespace NE
}; //namespace Exec

#endif /*!Exec_ExecNE_h*/
