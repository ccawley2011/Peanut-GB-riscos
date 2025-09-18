/**
 * The following header provides macros that allow
 * building with both the GNU assembler and objasm
 * using the same sources.
 */

#ifdef __CC_NORCROFT
        MACRO
        Label $label
$label
        MEND

#define TEXT AREA    |Asm$$Code|, CODE, READONLY
#define L(x) Label x
#else
#define TEXT .text
#define EXPORT .global
#define L(x) x:
#define END
#endif
