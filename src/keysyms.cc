#include "keysyms.h"

static struct ucs_keysym {
  unsigned short ucs;
  unsigned short keysym;
} ucs_keysyms[] = {
  { 0x0100, 0x03c0 }, /* Amacron */
  { 0x0101, 0x03e0 }, /* amacron */
  { 0x0102, 0x01c3 }, /* Abreve */
  { 0x0103, 0x01e3 }, /* abreve */
  { 0x0104, 0x01a1 }, /* Aogonek */
  { 0x0105, 0x01b1 }, /* aogonek */
  { 0x0106, 0x01c6 }, /* Cacute */
  { 0x0107, 0x01e6 }, /* cacute */
  { 0x0108, 0x02c6 }, /* Ccircumflex */
  { 0x0109, 0x02e6 }, /* ccircumflex */
  { 0x010a, 0x02c5 }, /* Cabovedot */
  { 0x010b, 0x02e5 }, /* cabovedot */
  { 0x010c, 0x01c8 }, /* Ccaron */
  { 0x010d, 0x01e8 }, /* ccaron */
  { 0x010e, 0x01cf }, /* Dcaron */
  { 0x010f, 0x01ef }, /* dcaron */
  { 0x0110, 0x01d0 }, /* Dstroke */
  { 0x0111, 0x01f0 }, /* dstroke */
  { 0x0112, 0x03aa }, /* Emacron */
  { 0x0113, 0x03ba }, /* emacron */
  { 0x0116, 0x03cc }, /* Eabovedot */
  { 0x0117, 0x03ec }, /* eabovedot */
  { 0x0118, 0x01ca }, /* Eogonek */
  { 0x0119, 0x01ea }, /* eogonek */
  { 0x011a, 0x01cc }, /* Ecaron */
  { 0x011b, 0x01ec }, /* ecaron */
  { 0x011c, 0x02d8 }, /* Gcircumflex */
  { 0x011d, 0x02f8 }, /* gcircumflex */
  { 0x011e, 0x02ab }, /* Gbreve */
  { 0x011f, 0x02bb }, /* gbreve */
  { 0x0120, 0x02d5 }, /* Gabovedot */
  { 0x0121, 0x02f5 }, /* gabovedot */
  { 0x0122, 0x03ab }, /* Gcedilla */
  { 0x0123, 0x03bb }, /* gcedilla */
  { 0x0124, 0x02a6 }, /* Hcircumflex */
  { 0x0125, 0x02b6 }, /* hcircumflex */
  { 0x0126, 0x02a1 }, /* Hstroke */
  { 0x0127, 0x02b1 }, /* hstroke */
  { 0x0128, 0x03a5 }, /* Itilde */
  { 0x0129, 0x03b5 }, /* itilde */
  { 0x012a, 0x03cf }, /* Imacron */
  { 0x012b, 0x03ef }, /* imacron */
  { 0x012e, 0x03c7 }, /* Iogonek */
  { 0x012f, 0x03e7 }, /* iogonek */
  { 0x0130, 0x02a9 }, /* Iabovedot */
  { 0x0131, 0x02b9 }, /* idotless */
  { 0x0134, 0x02ac }, /* Jcircumflex */
  { 0x0135, 0x02bc }, /* jcircumflex */
  { 0x0136, 0x03d3 }, /* Kcedilla */
  { 0x0137, 0x03f3 }, /* kcedilla */
  { 0x0138, 0x03a2 }, /* kra */
  { 0x0139, 0x01c5 }, /* Lacute */
  { 0x013a, 0x01e5 }, /* lacute */
  { 0x013b, 0x03a6 }, /* Lcedilla */
  { 0x013c, 0x03b6 }, /* lcedilla */
  { 0x013d, 0x01a5 }, /* Lcaron */
  { 0x013e, 0x01b5 }, /* lcaron */
  { 0x0141, 0x01a3 }, /* Lstroke */
  { 0x0142, 0x01b3 }, /* lstroke */
  { 0x0143, 0x01d1 }, /* Nacute */
  { 0x0144, 0x01f1 }, /* nacute */
  { 0x0145, 0x03d1 }, /* Ncedilla */
  { 0x0146, 0x03f1 }, /* ncedilla */
  { 0x0147, 0x01d2 }, /* Ncaron */
  { 0x0148, 0x01f2 }, /* ncaron */
  { 0x014a, 0x03bd }, /* ENG */
  { 0x014b, 0x03bf }, /* eng */
  { 0x014c, 0x03d2 }, /* Omacron */
  { 0x014d, 0x03f2 }, /* omacron */
  { 0x0150, 0x01d5 }, /* Odoubleacute */
  { 0x0151, 0x01f5 }, /* odoubleacute */
  { 0x0152, 0x13bc }, /* OE */
  { 0x0153, 0x13bd }, /* oe */
  { 0x0154, 0x01c0 }, /* Racute */
  { 0x0155, 0x01e0 }, /* racute */
  { 0x0156, 0x03a3 }, /* Rcedilla */
  { 0x0157, 0x03b3 }, /* rcedilla */
  { 0x0158, 0x01d8 }, /* Rcaron */
  { 0x0159, 0x01f8 }, /* rcaron */
  { 0x015a, 0x01a6 }, /* Sacute */
  { 0x015b, 0x01b6 }, /* sacute */
  { 0x015c, 0x02de }, /* Scircumflex */
  { 0x015d, 0x02fe }, /* scircumflex */
  { 0x015e, 0x01aa }, /* Scedilla */
  { 0x015f, 0x01ba }, /* scedilla */
  { 0x0160, 0x01a9 }, /* Scaron */
  { 0x0161, 0x01b9 }, /* scaron */
  { 0x0162, 0x01de }, /* Tcedilla */
  { 0x0163, 0x01fe }, /* tcedilla */
  { 0x0164, 0x01ab }, /* Tcaron */
  { 0x0165, 0x01bb }, /* tcaron */
  { 0x0166, 0x03ac }, /* Tslash */
  { 0x0167, 0x03bc }, /* tslash */
  { 0x0168, 0x03dd }, /* Utilde */
  { 0x0169, 0x03fd }, /* utilde */
  { 0x016a, 0x03de }, /* Umacron */
  { 0x016b, 0x03fe }, /* umacron */
  { 0x016c, 0x02dd }, /* Ubreve */
  { 0x016d, 0x02fd }, /* ubreve */
  { 0x016e, 0x01d9 }, /* Uring */
  { 0x016f, 0x01f9 }, /* uring */
  { 0x0170, 0x01db }, /* Udoubleacute */
  { 0x0171, 0x01fb }, /* udoubleacute */
  { 0x0172, 0x03d9 }, /* Uogonek */
  { 0x0173, 0x03f9 }, /* uogonek */
  { 0x0178, 0x13be }, /* Ydiaeresis */
  { 0x0179, 0x01ac }, /* Zacute */
  { 0x017a, 0x01bc }, /* zacute */
  { 0x017b, 0x01af }, /* Zabovedot */
  { 0x017c, 0x01bf }, /* zabovedot */
  { 0x017d, 0x01ae }, /* Zcaron */
  { 0x017e, 0x01be }, /* zcaron */
  { 0x0192, 0x08f6 }, /* function */
  { 0x02c7, 0x01b7 }, /* caron */
  { 0x02d8, 0x01a2 }, /* breve */
  { 0x02d9, 0x01ff }, /* abovedot */
  { 0x02db, 0x01b2 }, /* ogonek */
  { 0x02dd, 0x01bd }, /* doubleacute */
  { 0x0385, 0x07ae }, /* Greek_accentdieresis */
  { 0x0386, 0x07a1 }, /* Greek_ALPHAaccent */
  { 0x0388, 0x07a2 }, /* Greek_EPSILONaccent */
  { 0x0389, 0x07a3 }, /* Greek_ETAaccent */
  { 0x038a, 0x07a4 }, /* Greek_IOTAaccent */
  { 0x038c, 0x07a7 }, /* Greek_OMICRONaccent */
  { 0x038e, 0x07a8 }, /* Greek_UPSILONaccent */
  { 0x038f, 0x07ab }, /* Greek_OMEGAaccent */
  { 0x0390, 0x07b6 }, /* Greek_iotaaccentdieresis */
  { 0x0391, 0x07c1 }, /* Greek_ALPHA */
  { 0x0392, 0x07c2 }, /* Greek_BETA */
  { 0x0393, 0x07c3 }, /* Greek_GAMMA */
  { 0x0394, 0x07c4 }, /* Greek_DELTA */
  { 0x0395, 0x07c5 }, /* Greek_EPSILON */
  { 0x0396, 0x07c6 }, /* Greek_ZETA */
  { 0x0397, 0x07c7 }, /* Greek_ETA */
  { 0x0398, 0x07c8 }, /* Greek_THETA */
  { 0x0399, 0x07c9 }, /* Greek_IOTA */
  { 0x039a, 0x07ca }, /* Greek_KAPPA */
  { 0x039b, 0x07cb }, /* Greek_LAMBDA */
  { 0x039c, 0x07cc }, /* Greek_MU */
  { 0x039d, 0x07cd }, /* Greek_NU */
  { 0x039e, 0x07ce }, /* Greek_XI */
  { 0x039f, 0x07cf }, /* Greek_OMICRON */
  { 0x03a0, 0x07d0 }, /* Greek_PI */
  { 0x03a1, 0x07d1 }, /* Greek_RHO */
  { 0x03a3, 0x07d2 }, /* Greek_SIGMA */
  { 0x03a4, 0x07d4 }, /* Greek_TAU */
  { 0x03a5, 0x07d5 }, /* Greek_UPSILON */
  { 0x03a6, 0x07d6 }, /* Greek_PHI */
  { 0x03a7, 0x07d7 }, /* Greek_CHI */
  { 0x03a8, 0x07d8 }, /* Greek_PSI */
  { 0x03a9, 0x07d9 }, /* Greek_OMEGA */
  { 0x03aa, 0x07a5 }, /* Greek_IOTAdiaeresis */
  { 0x03ab, 0x07a9 }, /* Greek_UPSILONdieresis */
  { 0x03ac, 0x07b1 }, /* Greek_alphaaccent */
  { 0x03ad, 0x07b2 }, /* Greek_epsilonaccent */
  { 0x03ae, 0x07b3 }, /* Greek_etaaccent */
  { 0x03af, 0x07b4 }, /* Greek_iotaaccent */
  { 0x03b0, 0x07ba }, /* Greek_upsilonaccentdieresis */
  { 0x03b1, 0x07e1 }, /* Greek_alpha */
  { 0x03b2, 0x07e2 }, /* Greek_beta */
  { 0x03b3, 0x07e3 }, /* Greek_gamma */
  { 0x03b4, 0x07e4 }, /* Greek_delta */
  { 0x03b5, 0x07e5 }, /* Greek_epsilon */
  { 0x03b6, 0x07e6 }, /* Greek_zeta */
  { 0x03b7, 0x07e7 }, /* Greek_eta */
  { 0x03b8, 0x07e8 }, /* Greek_theta */
  { 0x03b9, 0x07e9 }, /* Greek_iota */
  { 0x03ba, 0x07ea }, /* Greek_kappa */
  { 0x03bb, 0x07eb }, /* Greek_lambda */
  { 0x03bc, 0x07ec }, /* Greek_mu */
  { 0x03bd, 0x07ed }, /* Greek_nu */
  { 0x03be, 0x07ee }, /* Greek_xi */
  { 0x03bf, 0x07ef }, /* Greek_omicron */
  { 0x03c0, 0x07f0 }, /* Greek_pi */
  { 0x03c1, 0x07f1 }, /* Greek_rho */
  { 0x03c2, 0x07f3 }, /* Greek_finalsmallsigma */
  { 0x03c3, 0x07f2 }, /* Greek_sigma */
  { 0x03c4, 0x07f4 }, /* Greek_tau */
  { 0x03c5, 0x07f5 }, /* Greek_upsilon */
  { 0x03c6, 0x07f6 }, /* Greek_phi */
  { 0x03c7, 0x07f7 }, /* Greek_chi */
  { 0x03c8, 0x07f8 }, /* Greek_psi */
  { 0x03c9, 0x07f9 }, /* Greek_omega */
  { 0x03ca, 0x07b5 }, /* Greek_iotadieresis */
  { 0x03cb, 0x07b9 }, /* Greek_upsilondieresis */
  { 0x03cc, 0x07b7 }, /* Greek_omicronaccent */
  { 0x03cd, 0x07b8 }, /* Greek_upsilonaccent */
  { 0x03ce, 0x07bb }, /* Greek_omegaaccent */
  { 0x0401, 0x06b3 }, /* Cyrillic_IO */
  { 0x0402, 0x06b1 }, /* Serbian_DJE */
  { 0x0403, 0x06b2 }, /* Macedonia_GJE */
  { 0x0404, 0x06b4 }, /* Ukrainian_IE */
  { 0x0405, 0x06b5 }, /* Macedonia_DSE */
  { 0x0406, 0x06b6 }, /* Ukrainian_I */
  { 0x0407, 0x06b7 }, /* Ukrainian_YI */
  { 0x0408, 0x06b8 }, /* Cyrillic_JE */
  { 0x0409, 0x06b9 }, /* Cyrillic_LJE */
  { 0x040a, 0x06ba }, /* Cyrillic_NJE */
  { 0x040b, 0x06bb }, /* Serbian_TSHE */
  { 0x040c, 0x06bc }, /* Macedonia_KJE */
  { 0x040e, 0x06be }, /* Byelorussian_SHORTU */
  { 0x040f, 0x06bf }, /* Cyrillic_DZHE */
  { 0x0410, 0x06e1 }, /* Cyrillic_A */
  { 0x0411, 0x06e2 }, /* Cyrillic_BE */
  { 0x0412, 0x06f7 }, /* Cyrillic_VE */
  { 0x0413, 0x06e7 }, /* Cyrillic_GHE */
  { 0x0414, 0x06e4 }, /* Cyrillic_DE */
  { 0x0415, 0x06e5 }, /* Cyrillic_IE */
  { 0x0416, 0x06f6 }, /* Cyrillic_ZHE */
  { 0x0417, 0x06fa }, /* Cyrillic_ZE */
  { 0x0418, 0x06e9 }, /* Cyrillic_I */
  { 0x0419, 0x06ea }, /* Cyrillic_SHORTI */
  { 0x041a, 0x06eb }, /* Cyrillic_KA */
  { 0x041b, 0x06ec }, /* Cyrillic_EL */
  { 0x041c, 0x06ed }, /* Cyrillic_EM */
  { 0x041d, 0x06ee }, /* Cyrillic_EN */
  { 0x041e, 0x06ef }, /* Cyrillic_O */
  { 0x041f, 0x06f0 }, /* Cyrillic_PE */
  { 0x0420, 0x06f2 }, /* Cyrillic_ER */
  { 0x0421, 0x06f3 }, /* Cyrillic_ES */
  { 0x0422, 0x06f4 }, /* Cyrillic_TE */
  { 0x0423, 0x06f5 }, /* Cyrillic_U */
  { 0x0424, 0x06e6 }, /* Cyrillic_EF */
  { 0x0425, 0x06e8 }, /* Cyrillic_HA */
  { 0x0426, 0x06e3 }, /* Cyrillic_TSE */
  { 0x0427, 0x06fe }, /* Cyrillic_CHE */
  { 0x0428, 0x06fb }, /* Cyrillic_SHA */
  { 0x0429, 0x06fd }, /* Cyrillic_SHCHA */
  { 0x042a, 0x06ff }, /* Cyrillic_HARDSIGN */
  { 0x042b, 0x06f9 }, /* Cyrillic_YERU */
  { 0x042c, 0x06f8 }, /* Cyrillic_SOFTSIGN */
  { 0x042d, 0x06fc }, /* Cyrillic_E */
  { 0x042e, 0x06e0 }, /* Cyrillic_YU */
  { 0x042f, 0x06f1 }, /* Cyrillic_YA */
  { 0x0430, 0x06c1 }, /* Cyrillic_a */
  { 0x0431, 0x06c2 }, /* Cyrillic_be */
  { 0x0432, 0x06d7 }, /* Cyrillic_ve */
  { 0x0433, 0x06c7 }, /* Cyrillic_ghe */
  { 0x0434, 0x06c4 }, /* Cyrillic_de */
  { 0x0435, 0x06c5 }, /* Cyrillic_ie */
  { 0x0436, 0x06d6 }, /* Cyrillic_zhe */
  { 0x0437, 0x06da }, /* Cyrillic_ze */
  { 0x0438, 0x06c9 }, /* Cyrillic_i */
  { 0x0439, 0x06ca }, /* Cyrillic_shorti */
  { 0x043a, 0x06cb }, /* Cyrillic_ka */
  { 0x043b, 0x06cc }, /* Cyrillic_el */
  { 0x043c, 0x06cd }, /* Cyrillic_em */
  { 0x043d, 0x06ce }, /* Cyrillic_en */
  { 0x043e, 0x06cf }, /* Cyrillic_o */
  { 0x043f, 0x06d0 }, /* Cyrillic_pe */
  { 0x0440, 0x06d2 }, /* Cyrillic_er */
  { 0x0441, 0x06d3 }, /* Cyrillic_es */
  { 0x0442, 0x06d4 }, /* Cyrillic_te */
  { 0x0443, 0x06d5 }, /* Cyrillic_u */
  { 0x0444, 0x06c6 }, /* Cyrillic_ef */
  { 0x0445, 0x06c8 }, /* Cyrillic_ha */
  { 0x0446, 0x06c3 }, /* Cyrillic_tse */
  { 0x0447, 0x06de }, /* Cyrillic_che */
  { 0x0448, 0x06db }, /* Cyrillic_sha */
  { 0x0449, 0x06dd }, /* Cyrillic_shcha */
  { 0x044a, 0x06df }, /* Cyrillic_hardsign */
  { 0x044b, 0x06d9 }, /* Cyrillic_yeru */
  { 0x044c, 0x06d8 }, /* Cyrillic_softsign */
  { 0x044d, 0x06dc }, /* Cyrillic_e */
  { 0x044e, 0x06c0 }, /* Cyrillic_yu */
  { 0x044f, 0x06d1 }, /* Cyrillic_ya */
  { 0x0451, 0x06a3 }, /* Cyrillic_io */
  { 0x0452, 0x06a1 }, /* Serbian_dje */
  { 0x0453, 0x06a2 }, /* Macedonia_gje */
  { 0x0454, 0x06a4 }, /* Ukrainian_ie */
  { 0x0455, 0x06a5 }, /* Macedonia_dse */
  { 0x0456, 0x06a6 }, /* Ukrainian_i */
  { 0x0457, 0x06a7 }, /* Ukrainian_yi */
  { 0x0458, 0x06a8 }, /* Cyrillic_je */
  { 0x0459, 0x06a9 }, /* Cyrillic_lje */
  { 0x045a, 0x06aa }, /* Cyrillic_nje */
  { 0x045b, 0x06ab }, /* Serbian_tshe */
  { 0x045c, 0x06ac }, /* Macedonia_kje */
  { 0x045e, 0x06ae }, /* Byelorussian_shortu */
  { 0x045f, 0x06af }, /* Cyrillic_dzhe */
  { 0x0490, 0x06bd }, /* Ukrainian_GHE_WITH_UPTURN */
  { 0x0491, 0x06ad }, /* Ukrainian_ghe_with_upturn */
  { 0x05d0, 0x0ce0 }, /* hebrew_aleph */
  { 0x05d1, 0x0ce1 }, /* hebrew_bet */
  { 0x05d2, 0x0ce2 }, /* hebrew_gimel */
  { 0x05d3, 0x0ce3 }, /* hebrew_dalet */
  { 0x05d4, 0x0ce4 }, /* hebrew_he */
  { 0x05d5, 0x0ce5 }, /* hebrew_waw */
  { 0x05d6, 0x0ce6 }, /* hebrew_zain */
  { 0x05d7, 0x0ce7 }, /* hebrew_chet */
  { 0x05d8, 0x0ce8 }, /* hebrew_tet */
  { 0x05d9, 0x0ce9 }, /* hebrew_yod */
  { 0x05da, 0x0cea }, /* hebrew_finalkaph */
  { 0x05db, 0x0ceb }, /* hebrew_kaph */
  { 0x05dc, 0x0cec }, /* hebrew_lamed */
  { 0x05dd, 0x0ced }, /* hebrew_finalmem */
  { 0x05de, 0x0cee }, /* hebrew_mem */
  { 0x05df, 0x0cef }, /* hebrew_finalnun */
  { 0x05e0, 0x0cf0 }, /* hebrew_nun */
  { 0x05e1, 0x0cf1 }, /* hebrew_samech */
  { 0x05e2, 0x0cf2 }, /* hebrew_ayin */
  { 0x05e3, 0x0cf3 }, /* hebrew_finalpe */
  { 0x05e4, 0x0cf4 }, /* hebrew_pe */
  { 0x05e5, 0x0cf5 }, /* hebrew_finalzade */
  { 0x05e6, 0x0cf6 }, /* hebrew_zade */
  { 0x05e7, 0x0cf7 }, /* hebrew_qoph */
  { 0x05e8, 0x0cf8 }, /* hebrew_resh */
  { 0x05e9, 0x0cf9 }, /* hebrew_shin */
  { 0x05ea, 0x0cfa }, /* hebrew_taw */
  { 0x060c, 0x05ac }, /* Arabic_comma */
  { 0x061b, 0x05bb }, /* Arabic_semicolon */
  { 0x061f, 0x05bf }, /* Arabic_question_mark */
  { 0x0621, 0x05c1 }, /* Arabic_hamza */
  { 0x0622, 0x05c2 }, /* Arabic_maddaonalef */
  { 0x0623, 0x05c3 }, /* Arabic_hamzaonalef */
  { 0x0624, 0x05c4 }, /* Arabic_hamzaonwaw */
  { 0x0625, 0x05c5 }, /* Arabic_hamzaunderalef */
  { 0x0626, 0x05c6 }, /* Arabic_hamzaonyeh */
  { 0x0627, 0x05c7 }, /* Arabic_alef */
  { 0x0628, 0x05c8 }, /* Arabic_beh */
  { 0x0629, 0x05c9 }, /* Arabic_tehmarbuta */
  { 0x062a, 0x05ca }, /* Arabic_teh */
  { 0x062b, 0x05cb }, /* Arabic_theh */
  { 0x062c, 0x05cc }, /* Arabic_jeem */
  { 0x062d, 0x05cd }, /* Arabic_hah */
  { 0x062e, 0x05ce }, /* Arabic_khah */
  { 0x062f, 0x05cf }, /* Arabic_dal */
  { 0x0630, 0x05d0 }, /* Arabic_thal */
  { 0x0631, 0x05d1 }, /* Arabic_ra */
  { 0x0632, 0x05d2 }, /* Arabic_zain */
  { 0x0633, 0x05d3 }, /* Arabic_seen */
  { 0x0634, 0x05d4 }, /* Arabic_sheen */
  { 0x0635, 0x05d5 }, /* Arabic_sad */
  { 0x0636, 0x05d6 }, /* Arabic_dad */
  { 0x0637, 0x05d7 }, /* Arabic_tah */
  { 0x0638, 0x05d8 }, /* Arabic_zah */
  { 0x0639, 0x05d9 }, /* Arabic_ain */
  { 0x063a, 0x05da }, /* Arabic_ghain */
  { 0x0640, 0x05e0 }, /* Arabic_tatweel */
  { 0x0641, 0x05e1 }, /* Arabic_feh */
  { 0x0642, 0x05e2 }, /* Arabic_qaf */
  { 0x0643, 0x05e3 }, /* Arabic_kaf */
  { 0x0644, 0x05e4 }, /* Arabic_lam */
  { 0x0645, 0x05e5 }, /* Arabic_meem */
  { 0x0646, 0x05e6 }, /* Arabic_noon */
  { 0x0647, 0x05e7 }, /* Arabic_ha */
  { 0x0648, 0x05e8 }, /* Arabic_waw */
  { 0x0649, 0x05e9 }, /* Arabic_alefmaksura */
  { 0x064a, 0x05ea }, /* Arabic_yeh */
  { 0x064b, 0x05eb }, /* Arabic_fathatan */
  { 0x064c, 0x05ec }, /* Arabic_dammatan */
  { 0x064d, 0x05ed }, /* Arabic_kasratan */
  { 0x064e, 0x05ee }, /* Arabic_fatha */
  { 0x064f, 0x05ef }, /* Arabic_damma */
  { 0x0650, 0x05f0 }, /* Arabic_kasra */
  { 0x0651, 0x05f1 }, /* Arabic_shadda */
  { 0x0652, 0x05f2 }, /* Arabic_sukun */
  { 0x0e01, 0x0da1 }, /* Thai_kokai */
  { 0x0e02, 0x0da2 }, /* Thai_khokhai */
  { 0x0e03, 0x0da3 }, /* Thai_khokhuat */
  { 0x0e04, 0x0da4 }, /* Thai_khokhwai */
  { 0x0e05, 0x0da5 }, /* Thai_khokhon */
  { 0x0e06, 0x0da6 }, /* Thai_khorakhang */
  { 0x0e07, 0x0da7 }, /* Thai_ngongu */
  { 0x0e08, 0x0da8 }, /* Thai_chochan */
  { 0x0e09, 0x0da9 }, /* Thai_choching */
  { 0x0e0a, 0x0daa }, /* Thai_chochang */
  { 0x0e0b, 0x0dab }, /* Thai_soso */
  { 0x0e0c, 0x0dac }, /* Thai_chochoe */
  { 0x0e0d, 0x0dad }, /* Thai_yoying */
  { 0x0e0e, 0x0dae }, /* Thai_dochada */
  { 0x0e0f, 0x0daf }, /* Thai_topatak */
  { 0x0e10, 0x0db0 }, /* Thai_thothan */
  { 0x0e11, 0x0db1 }, /* Thai_thonangmontho */
  { 0x0e12, 0x0db2 }, /* Thai_thophuthao */
  { 0x0e13, 0x0db3 }, /* Thai_nonen */
  { 0x0e14, 0x0db4 }, /* Thai_dodek */
  { 0x0e15, 0x0db5 }, /* Thai_totao */
  { 0x0e16, 0x0db6 }, /* Thai_thothung */
  { 0x0e17, 0x0db7 }, /* Thai_thothahan */
  { 0x0e18, 0x0db8 }, /* Thai_thothong */
  { 0x0e19, 0x0db9 }, /* Thai_nonu */
  { 0x0e1a, 0x0dba }, /* Thai_bobaimai */
  { 0x0e1b, 0x0dbb }, /* Thai_popla */
  { 0x0e1c, 0x0dbc }, /* Thai_phophung */
  { 0x0e1d, 0x0dbd }, /* Thai_fofa */
  { 0x0e1e, 0x0dbe }, /* Thai_phophan */
  { 0x0e1f, 0x0dbf }, /* Thai_fofan */
  { 0x0e20, 0x0dc0 }, /* Thai_phosamphao */
  { 0x0e21, 0x0dc1 }, /* Thai_moma */
  { 0x0e22, 0x0dc2 }, /* Thai_yoyak */
  { 0x0e23, 0x0dc3 }, /* Thai_rorua */
  { 0x0e24, 0x0dc4 }, /* Thai_ru */
  { 0x0e25, 0x0dc5 }, /* Thai_loling */
  { 0x0e26, 0x0dc6 }, /* Thai_lu */
  { 0x0e27, 0x0dc7 }, /* Thai_wowaen */
  { 0x0e28, 0x0dc8 }, /* Thai_sosala */
  { 0x0e29, 0x0dc9 }, /* Thai_sorusi */
  { 0x0e2a, 0x0dca }, /* Thai_sosua */
  { 0x0e2b, 0x0dcb }, /* Thai_hohip */
  { 0x0e2c, 0x0dcc }, /* Thai_lochula */
  { 0x0e2d, 0x0dcd }, /* Thai_oang */
  { 0x0e2e, 0x0dce }, /* Thai_honokhuk */
  { 0x0e2f, 0x0dcf }, /* Thai_paiyannoi */
  { 0x0e30, 0x0dd0 }, /* Thai_saraa */
  { 0x0e31, 0x0dd1 }, /* Thai_maihanakat */
  { 0x0e32, 0x0dd2 }, /* Thai_saraaa */
  { 0x0e33, 0x0dd3 }, /* Thai_saraam */
  { 0x0e34, 0x0dd4 }, /* Thai_sarai */
  { 0x0e35, 0x0dd5 }, /* Thai_saraii */
  { 0x0e36, 0x0dd6 }, /* Thai_saraue */
  { 0x0e37, 0x0dd7 }, /* Thai_sarauee */
  { 0x0e38, 0x0dd8 }, /* Thai_sarau */
  { 0x0e39, 0x0dd9 }, /* Thai_sarauu */
  { 0x0e3a, 0x0dda }, /* Thai_phinthu */
  { 0x0e3e, 0x0dde }, /* Thai_maihanakat_maitho */
  { 0x0e3f, 0x0ddf }, /* Thai_baht */
  { 0x0e40, 0x0de0 }, /* Thai_sarae */
  { 0x0e41, 0x0de1 }, /* Thai_saraae */
  { 0x0e42, 0x0de2 }, /* Thai_sarao */
  { 0x0e43, 0x0de3 }, /* Thai_saraaimaimuan */
  { 0x0e44, 0x0de4 }, /* Thai_saraaimaimalai */
  { 0x0e45, 0x0de5 }, /* Thai_lakkhangyao */
  { 0x0e46, 0x0de6 }, /* Thai_maiyamok */
  { 0x0e47, 0x0de7 }, /* Thai_maitaikhu */
  { 0x0e48, 0x0de8 }, /* Thai_maiek */
  { 0x0e49, 0x0de9 }, /* Thai_maitho */
  { 0x0e4a, 0x0dea }, /* Thai_maitri */
  { 0x0e4b, 0x0deb }, /* Thai_maichattawa */
  { 0x0e4c, 0x0dec }, /* Thai_thanthakhat */
  { 0x0e4d, 0x0ded }, /* Thai_nikhahit */
  { 0x0e50, 0x0df0 }, /* Thai_leksun */
  { 0x0e51, 0x0df1 }, /* Thai_leknung */
  { 0x0e52, 0x0df2 }, /* Thai_leksong */
  { 0x0e53, 0x0df3 }, /* Thai_leksam */
  { 0x0e54, 0x0df4 }, /* Thai_leksi */
  { 0x0e55, 0x0df5 }, /* Thai_lekha */
  { 0x0e56, 0x0df6 }, /* Thai_lekhok */
  { 0x0e57, 0x0df7 }, /* Thai_lekchet */
  { 0x0e58, 0x0df8 }, /* Thai_lekpaet */
  { 0x0e59, 0x0df9 }, /* Thai_lekkao */
  { 0x11a8, 0x0ed4 }, /* Hangul_J_Kiyeog */
  { 0x11a9, 0x0ed5 }, /* Hangul_J_SsangKiyeog */
  { 0x11aa, 0x0ed6 }, /* Hangul_J_KiyeogSios */
  { 0x11ab, 0x0ed7 }, /* Hangul_J_Nieun */
  { 0x11ac, 0x0ed8 }, /* Hangul_J_NieunJieuj */
  { 0x11ad, 0x0ed9 }, /* Hangul_J_NieunHieuh */
  { 0x11ae, 0x0eda }, /* Hangul_J_Dikeud */
  { 0x11af, 0x0edb }, /* Hangul_J_Rieul */
  { 0x11b0, 0x0edc }, /* Hangul_J_RieulKiyeog */
  { 0x11b1, 0x0edd }, /* Hangul_J_RieulMieum */
  { 0x11b2, 0x0ede }, /* Hangul_J_RieulPieub */
  { 0x11b3, 0x0edf }, /* Hangul_J_RieulSios */
  { 0x11b4, 0x0ee0 }, /* Hangul_J_RieulTieut */
  { 0x11b5, 0x0ee1 }, /* Hangul_J_RieulPhieuf */
  { 0x11b6, 0x0ee2 }, /* Hangul_J_RieulHieuh */
  { 0x11b7, 0x0ee3 }, /* Hangul_J_Mieum */
  { 0x11b8, 0x0ee4 }, /* Hangul_J_Pieub */
  { 0x11b9, 0x0ee5 }, /* Hangul_J_PieubSios */
  { 0x11ba, 0x0ee6 }, /* Hangul_J_Sios */
  { 0x11bb, 0x0ee7 }, /* Hangul_J_SsangSios */
  { 0x11bc, 0x0ee8 }, /* Hangul_J_Ieung */
  { 0x11bd, 0x0ee9 }, /* Hangul_J_Jieuj */
  { 0x11be, 0x0eea }, /* Hangul_J_Cieuc */
  { 0x11bf, 0x0eeb }, /* Hangul_J_Khieuq */
  { 0x11c0, 0x0eec }, /* Hangul_J_Tieut */
  { 0x11c1, 0x0eed }, /* Hangul_J_Phieuf */
  { 0x11c2, 0x0eee }, /* Hangul_J_Hieuh */
  { 0x11eb, 0x0ef8 }, /* Hangul_J_PanSios */
  { 0x11f9, 0x0efa }, /* Hangul_J_YeorinHieuh */
  { 0x2002, 0x0aa2 }, /* enspace */
  { 0x2003, 0x0aa1 }, /* emspace */
  { 0x2004, 0x0aa3 }, /* em3space */
  { 0x2005, 0x0aa4 }, /* em4space */
  { 0x2007, 0x0aa5 }, /* digitspace */
  { 0x2008, 0x0aa6 }, /* punctspace */
  { 0x2009, 0x0aa7 }, /* thinspace */
  { 0x200a, 0x0aa8 }, /* hairspace */
  { 0x2012, 0x0abb }, /* figdash */
  { 0x2013, 0x0aaa }, /* endash */
  { 0x2014, 0x0aa9 }, /* emdash */
  { 0x2015, 0x07af }, /* Greek_horizbar */
  { 0x2017, 0x0cdf }, /* hebrew_doublelowline */
  { 0x2018, 0x0ad0 }, /* leftsinglequotemark */
  { 0x2019, 0x0ad1 }, /* rightsinglequotemark */
  { 0x201a, 0x0afd }, /* singlelowquotemark */
  { 0x201c, 0x0ad2 }, /* leftdoublequotemark */
  { 0x201d, 0x0ad3 }, /* rightdoublequotemark */
  { 0x201e, 0x0afe }, /* doublelowquotemark */
  { 0x2020, 0x0af1 }, /* dagger */
  { 0x2021, 0x0af2 }, /* doubledagger */
  { 0x2022, 0x0ae6 }, /* enfilledcircbullet */
  { 0x2025, 0x0aaf }, /* doubbaselinedot */
  { 0x2026, 0x0aae }, /* ellipsis */
  { 0x2030, 0x0ad5 }, /* permille */
  { 0x2032, 0x0ad6 }, /* minutes */
  { 0x2033, 0x0ad7 }, /* seconds */
  { 0x2038, 0x0afc }, /* caret */
  { 0x203e, 0x047e }, /* overline */
  { 0x20a0, 0x20a0 }, /* EcuSign */
  { 0x20a1, 0x20a1 }, /* ColonSign */
  { 0x20a2, 0x20a2 }, /* CruzeiroSign */
  { 0x20a3, 0x20a3 }, /* FFrancSign */
  { 0x20a4, 0x20a4 }, /* LiraSign */
  { 0x20a5, 0x20a5 }, /* MillSign */
  { 0x20a6, 0x20a6 }, /* NairaSign */
  { 0x20a7, 0x20a7 }, /* PesetaSign */
  { 0x20a8, 0x20a8 }, /* RupeeSign */
  { 0x20a9, 0x0eff }, /* Korean_Won */
  { 0x20aa, 0x20aa }, /* NewSheqelSign */
  { 0x20ab, 0x20ab }, /* DongSign */
  { 0x20ac, 0x20ac }, /* EuroSign */
  { 0x2105, 0x0ab8 }, /* careof */
  { 0x2116, 0x06b0 }, /* numerosign */
  { 0x2117, 0x0afb }, /* phonographcopyright */
  { 0x211e, 0x0ad4 }, /* prescription */
  { 0x2122, 0x0ac9 }, /* trademark */
  { 0x2153, 0x0ab0 }, /* onethird */
  { 0x2154, 0x0ab1 }, /* twothirds */
  { 0x2155, 0x0ab2 }, /* onefifth */
  { 0x2156, 0x0ab3 }, /* twofifths */
  { 0x2157, 0x0ab4 }, /* threefifths */
  { 0x2158, 0x0ab5 }, /* fourfifths */
  { 0x2159, 0x0ab6 }, /* onesixth */
  { 0x215a, 0x0ab7 }, /* fivesixths */
  { 0x215b, 0x0ac3 }, /* oneeighth */
  { 0x215c, 0x0ac4 }, /* threeeighths */
  { 0x215d, 0x0ac5 }, /* fiveeighths */
  { 0x215e, 0x0ac6 }, /* seveneighths */
  { 0x2190, 0x08fb }, /* leftarrow */
  { 0x2191, 0x08fc }, /* uparrow */
  { 0x2192, 0x08fd }, /* rightarrow */
  { 0x2193, 0x08fe }, /* downarrow */
  { 0x21d2, 0x08ce }, /* implies */
  { 0x21d4, 0x08cd }, /* ifonlyif */
  { 0x2202, 0x08ef }, /* partialderivative */
  { 0x2207, 0x08c5 }, /* nabla */
  { 0x2218, 0x0bca }, /* jot */
  { 0x221a, 0x08d6 }, /* radical */
  { 0x221d, 0x08c1 }, /* variation */
  { 0x221e, 0x08c2 }, /* infinity */
  { 0x2227, 0x08de }, /* logicaland */
  { 0x2228, 0x08df }, /* logicalor */
  { 0x2229, 0x08dc }, /* intersection */
  { 0x222a, 0x08dd }, /* union */
  { 0x222b, 0x08bf }, /* integral */
  { 0x2234, 0x08c0 }, /* therefore */
  { 0x223c, 0x08c8 }, /* approximate */
  { 0x2243, 0x08c9 }, /* similarequal */
  { 0x2245, 0x08c8 }, /* approximate */
  { 0x2260, 0x08bd }, /* notequal */
  { 0x2261, 0x08cf }, /* identical */
  { 0x2264, 0x08bc }, /* lessthanequal */
  { 0x2265, 0x08be }, /* greaterthanequal */
  { 0x2282, 0x08da }, /* includedin */
  { 0x2283, 0x08db }, /* includes */
  { 0x22a2, 0x0bfc }, /* righttack */
  { 0x22a3, 0x0bdc }, /* lefttack */
  { 0x22a4, 0x0bc2 }, /* downtack */
  { 0x22a5, 0x0bce }, /* uptack */
  { 0x2308, 0x0bd3 }, /* upstile */
  { 0x230a, 0x0bc4 }, /* downstile */
  { 0x2315, 0x0afa }, /* telephonerecorder */
  { 0x2320, 0x08a4 }, /* topintegral */
  { 0x2321, 0x08a5 }, /* botintegral */
  { 0x2329, 0x0abc }, /* leftanglebracket */
  { 0x232a, 0x0abe }, /* rightanglebracket */
  { 0x2395, 0x0bcc }, /* quad */
  { 0x239b, 0x08ab }, /* topleftparens */
  { 0x239d, 0x08ac }, /* botleftparens */
  { 0x239e, 0x08ad }, /* toprightparens */
  { 0x23a0, 0x08ae }, /* botrightparens */
  { 0x23a1, 0x08a7 }, /* topleftsqbracket */
  { 0x23a3, 0x08a8 }, /* botleftsqbracket */
  { 0x23a4, 0x08a9 }, /* toprightsqbracket */
  { 0x23a6, 0x08aa }, /* botrightsqbracket */
  { 0x23a8, 0x08af }, /* leftmiddlecurlybrace */
  { 0x23ac, 0x08b0 }, /* rightmiddlecurlybrace */
  { 0x23b7, 0x08a1 }, /* leftradical */
  { 0x23ba, 0x09ef }, /* horizlinescan1 */
  { 0x23bb, 0x09f0 }, /* horizlinescan3 */
  { 0x23bc, 0x09f2 }, /* horizlinescan7 */
  { 0x23bd, 0x09f3 }, /* horizlinescan9 */
  { 0x2409, 0x09e2 }, /* ht */
  { 0x240a, 0x09e5 }, /* lf */
  { 0x240b, 0x09e9 }, /* vt */
  { 0x240c, 0x09e3 }, /* ff */
  { 0x240d, 0x09e4 }, /* cr */
  { 0x2422, 0x09df }, /* blank */
  { 0x2424, 0x09e8 }, /* nl */
  { 0x2500, 0x09f1 }, /* horizlinescan5 */
  { 0x2502, 0x08a6 }, /* vertconnector */
  { 0x250c, 0x09ec }, /* upleftcorner */
  { 0x2510, 0x09eb }, /* uprightcorner */
  { 0x2514, 0x09ed }, /* lowleftcorner */
  { 0x2518, 0x09ea }, /* lowrightcorner */
  { 0x251c, 0x09f4 }, /* leftt */
  { 0x2524, 0x09f5 }, /* rightt */
  { 0x252c, 0x09f7 }, /* topt */
  { 0x2534, 0x09f6 }, /* bott */
  { 0x253c, 0x09ee }, /* crossinglines */
  { 0x2592, 0x09e1 }, /* checkerboard */
  { 0x25a0, 0x0adf }, /* emfilledrect */
  { 0x25a1, 0x0acf }, /* emopenrectangle */
  { 0x25aa, 0x0ae7 }, /* enfilledsqbullet */
  { 0x25ab, 0x0ae1 }, /* enopensquarebullet */
  { 0x25ac, 0x0adb }, /* filledrectbullet */
  { 0x25ad, 0x0ae2 }, /* openrectbullet */
  { 0x25b2, 0x0ae8 }, /* filledtribulletup */
  { 0x25b3, 0x0ae3 }, /* opentribulletup */
  { 0x25b6, 0x0add }, /* filledrighttribullet */
  { 0x25b7, 0x0acd }, /* rightopentriangle */
  { 0x25bc, 0x0ae9 }, /* filledtribulletdown */
  { 0x25bd, 0x0ae4 }, /* opentribulletdown */
  { 0x25c0, 0x0adc }, /* filledlefttribullet */
  { 0x25c1, 0x0acc }, /* leftopentriangle */
  { 0x25c6, 0x09e0 }, /* soliddiamond */
  { 0x25cb, 0x0ace }, /* emopencircle */
  { 0x25cf, 0x0ade }, /* emfilledcircle */
  { 0x25e6, 0x0ae0 }, /* enopencircbullet */
  { 0x2606, 0x0ae5 }, /* openstar */
  { 0x260e, 0x0af9 }, /* telephone */
  { 0x2613, 0x0aca }, /* signaturemark */
  { 0x261c, 0x0aea }, /* leftpointer */
  { 0x261e, 0x0aeb }, /* rightpointer */
  { 0x2640, 0x0af8 }, /* femalesymbol */
  { 0x2642, 0x0af7 }, /* malesymbol */
  { 0x2663, 0x0aec }, /* club */
  { 0x2665, 0x0aee }, /* heart */
  { 0x2666, 0x0aed }, /* diamond */
  { 0x266d, 0x0af6 }, /* musicalflat */
  { 0x266f, 0x0af5 }, /* musicalsharp */
  { 0x2713, 0x0af3 }, /* checkmark */
  { 0x2717, 0x0af4 }, /* ballotcross */
  { 0x271d, 0x0ad9 }, /* latincross */
  { 0x2720, 0x0af0 }, /* maltesecross */
  { 0x3001, 0x04a4 }, /* kana_comma */
  { 0x3002, 0x04a1 }, /* kana_fullstop */
  { 0x300c, 0x04a2 }, /* kana_openingbracket */
  { 0x300d, 0x04a3 }, /* kana_closingbracket */
  { 0x309b, 0x04de }, /* voicedsound */
  { 0x309c, 0x04df }, /* semivoicedsound */
  { 0x30a1, 0x04a7 }, /* kana_a */
  { 0x30a2, 0x04b1 }, /* kana_A */
  { 0x30a3, 0x04a8 }, /* kana_i */
  { 0x30a4, 0x04b2 }, /* kana_I */
  { 0x30a5, 0x04a9 }, /* kana_u */
  { 0x30a6, 0x04b3 }, /* kana_U */
  { 0x30a7, 0x04aa }, /* kana_e */
  { 0x30a8, 0x04b4 }, /* kana_E */
  { 0x30a9, 0x04ab }, /* kana_o */
  { 0x30aa, 0x04b5 }, /* kana_O */
  { 0x30ab, 0x04b6 }, /* kana_KA */
  { 0x30ad, 0x04b7 }, /* kana_KI */
  { 0x30af, 0x04b8 }, /* kana_KU */
  { 0x30b1, 0x04b9 }, /* kana_KE */
  { 0x30b3, 0x04ba }, /* kana_KO */
  { 0x30b5, 0x04bb }, /* kana_SA */
  { 0x30b7, 0x04bc }, /* kana_SHI */
  { 0x30b9, 0x04bd }, /* kana_SU */
  { 0x30bb, 0x04be }, /* kana_SE */
  { 0x30bd, 0x04bf }, /* kana_SO */
  { 0x30bf, 0x04c0 }, /* kana_TA */
  { 0x30c1, 0x04c1 }, /* kana_CHI */
  { 0x30c3, 0x04af }, /* kana_tsu */
  { 0x30c4, 0x04c2 }, /* kana_TSU */
  { 0x30c6, 0x04c3 }, /* kana_TE */
  { 0x30c8, 0x04c4 }, /* kana_TO */
  { 0x30ca, 0x04c5 }, /* kana_NA */
  { 0x30cb, 0x04c6 }, /* kana_NI */
  { 0x30cc, 0x04c7 }, /* kana_NU */
  { 0x30cd, 0x04c8 }, /* kana_NE */
  { 0x30ce, 0x04c9 }, /* kana_NO */
  { 0x30cf, 0x04ca }, /* kana_HA */
  { 0x30d2, 0x04cb }, /* kana_HI */
  { 0x30d5, 0x04cc }, /* kana_FU */
  { 0x30d8, 0x04cd }, /* kana_HE */
  { 0x30db, 0x04ce }, /* kana_HO */
  { 0x30de, 0x04cf }, /* kana_MA */
  { 0x30df, 0x04d0 }, /* kana_MI */
  { 0x30e0, 0x04d1 }, /* kana_MU */
  { 0x30e1, 0x04d2 }, /* kana_ME */
  { 0x30e2, 0x04d3 }, /* kana_MO */
  { 0x30e3, 0x04ac }, /* kana_ya */
  { 0x30e4, 0x04d4 }, /* kana_YA */
  { 0x30e5, 0x04ad }, /* kana_yu */
  { 0x30e6, 0x04d5 }, /* kana_YU */
  { 0x30e7, 0x04ae }, /* kana_yo */
  { 0x30e8, 0x04d6 }, /* kana_YO */
  { 0x30e9, 0x04d7 }, /* kana_RA */
  { 0x30ea, 0x04d8 }, /* kana_RI */
  { 0x30eb, 0x04d9 }, /* kana_RU */
  { 0x30ec, 0x04da }, /* kana_RE */
  { 0x30ed, 0x04db }, /* kana_RO */
  { 0x30ef, 0x04dc }, /* kana_WA */
  { 0x30f2, 0x04a6 }, /* kana_WO */
  { 0x30f3, 0x04dd }, /* kana_N */
  { 0x30fb, 0x04a5 }, /* kana_conjunctive */
  { 0x30fc, 0x04b0 }, /* prolongedsound */
  { 0x3131, 0x0ea1 }, /* Hangul_Kiyeog */
  { 0x3132, 0x0ea2 }, /* Hangul_SsangKiyeog */
  { 0x3133, 0x0ea3 }, /* Hangul_KiyeogSios */
  { 0x3134, 0x0ea4 }, /* Hangul_Nieun */
  { 0x3135, 0x0ea5 }, /* Hangul_NieunJieuj */
  { 0x3136, 0x0ea6 }, /* Hangul_NieunHieuh */
  { 0x3137, 0x0ea7 }, /* Hangul_Dikeud */
  { 0x3138, 0x0ea8 }, /* Hangul_SsangDikeud */
  { 0x3139, 0x0ea9 }, /* Hangul_Rieul */
  { 0x313a, 0x0eaa }, /* Hangul_RieulKiyeog */
  { 0x313b, 0x0eab }, /* Hangul_RieulMieum */
  { 0x313c, 0x0eac }, /* Hangul_RieulPieub */
  { 0x313d, 0x0ead }, /* Hangul_RieulSios */
  { 0x313e, 0x0eae }, /* Hangul_RieulTieut */
  { 0x313f, 0x0eaf }, /* Hangul_RieulPhieuf */
  { 0x3140, 0x0eb0 }, /* Hangul_RieulHieuh */
  { 0x3141, 0x0eb1 }, /* Hangul_Mieum */
  { 0x3142, 0x0eb2 }, /* Hangul_Pieub */
  { 0x3143, 0x0eb3 }, /* Hangul_SsangPieub */
  { 0x3144, 0x0eb4 }, /* Hangul_PieubSios */
  { 0x3145, 0x0eb5 }, /* Hangul_Sios */
  { 0x3146, 0x0eb6 }, /* Hangul_SsangSios */
  { 0x3147, 0x0eb7 }, /* Hangul_Ieung */
  { 0x3148, 0x0eb8 }, /* Hangul_Jieuj */
  { 0x3149, 0x0eb9 }, /* Hangul_SsangJieuj */
  { 0x314a, 0x0eba }, /* Hangul_Cieuc */
  { 0x314b, 0x0ebb }, /* Hangul_Khieuq */
  { 0x314c, 0x0ebc }, /* Hangul_Tieut */
  { 0x314d, 0x0ebd }, /* Hangul_Phieuf */
  { 0x314e, 0x0ebe }, /* Hangul_Hieuh */
  { 0x314f, 0x0ebf }, /* Hangul_A */
  { 0x3150, 0x0ec0 }, /* Hangul_AE */
  { 0x3151, 0x0ec1 }, /* Hangul_YA */
  { 0x3152, 0x0ec2 }, /* Hangul_YAE */
  { 0x3153, 0x0ec3 }, /* Hangul_EO */
  { 0x3154, 0x0ec4 }, /* Hangul_E */
  { 0x3155, 0x0ec5 }, /* Hangul_YEO */
  { 0x3156, 0x0ec6 }, /* Hangul_YE */
  { 0x3157, 0x0ec7 }, /* Hangul_O */
  { 0x3158, 0x0ec8 }, /* Hangul_WA */
  { 0x3159, 0x0ec9 }, /* Hangul_WAE */
  { 0x315a, 0x0eca }, /* Hangul_OE */
  { 0x315b, 0x0ecb }, /* Hangul_YO */
  { 0x315c, 0x0ecc }, /* Hangul_U */
  { 0x315d, 0x0ecd }, /* Hangul_WEO */
  { 0x315e, 0x0ece }, /* Hangul_WE */
  { 0x315f, 0x0ecf }, /* Hangul_WI */
  { 0x3160, 0x0ed0 }, /* Hangul_YU */
  { 0x3161, 0x0ed1 }, /* Hangul_EU */
  { 0x3162, 0x0ed2 }, /* Hangul_YI */
  { 0x3163, 0x0ed3 }, /* Hangul_I */
  { 0x316d, 0x0eef }, /* Hangul_RieulYeorinHieuh */
  { 0x3171, 0x0ef0 }, /* Hangul_SunkyeongeumMieum */
  { 0x3178, 0x0ef1 }, /* Hangul_SunkyeongeumPieub */
  { 0x317f, 0x0ef2 }, /* Hangul_PanSios */
  { 0x3184, 0x0ef4 }, /* Hangul_SunkyeongeumPhieuf */
  { 0x3186, 0x0ef5 }, /* Hangul_YeorinHieuh */
  { 0x318d, 0x0ef6 }, /* Hangul_AraeA */
  { 0x318e, 0x0ef7 }, /* Hangul_AraeAE */
};

unsigned short ucsToKeysym(int ucs) {
    if (ucs >= 0x0100 && ucs <= 0x318e) {
        int lo = 0;
        int hi = sizeof(ucs_keysyms) / sizeof(ucs_keysym);

        while (hi > lo) {
            int pv = (lo + hi) / 2;
            if (ucs_keysyms[pv].ucs < ucs)
                lo = pv + 1;
            else if (ucs_keysyms[pv].ucs > ucs)
                hi = pv;
            else
                return ucs_keysyms[pv].keysym;
        }
    }

    return 0;
}