#ifndef KEYSYMS_H
#define KEYSYMS_H

#define IS_XF86KEY(key)  (0x1008FE01U <= key && key <= 0x1008FFFFU)
#define IS_POINTER(key)  (0x0000FEE0U <= key && key <= 0x0000FEFFU)
#define IS_MODIFIER(key) (0x0000FFE1U <= key && key <= 0x0000FFEEU)

unsigned short ucsToKeysym(int ucs);
unsigned mapKeypad(unsigned keysym);

#endif
