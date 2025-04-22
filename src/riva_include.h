#ifndef __RIVA_INCLUDE_H__
#define __RIVA_INCLUDE_H__

#include <string.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

#include "compiler.h"

/* Drivers for PCI hardware need this */
#include "pci_ids.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

#include "micmap.h"

#include "xf86DDC.h"

#include "vbe.h"


#include "riva_const.h"

#include "dixstruct.h"
#include "scrnintstr.h"

#include "fb.h"

#include "xf86fbman.h"
#include "xf86cmap.h"
#include "shadowfb.h"
#include "fbdevhw.h"

#include "xf86xv.h"
#include <X11/extensions/Xv.h>

#include "vgaHW.h"

#include "xf86Cursor.h"

#include "region.h"

#include "compat-api.h"
#include "riva_local.h"
#include "riva_type.h"
#include "riva_proto.h"

#endif /* __RIVA_INCLUDE_H__ */
