#define CU_MASK  (0)

#define CU_BPP   (2)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (3)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (4)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK _
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 0
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#undef  CU_MASK

#define CU_MASK  (1)

#define CU_BPP   (2)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (3)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (4)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 10  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK m
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 1
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#undef  CU_MASK

#define CU_MASK  (2)

#define CU_BPP   (2)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 2
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (3)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 3
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#define CU_BPP   (4)

#define CU_SIZE  (8)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 8
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 8  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (16)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 16
#define CTV_ROWS r
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#define CU_SIZE  (32)

#define CU_ROWS  (0)

#define CU_CARE  (0)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE _
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX _
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#define CU_FLIPX (1)
#define CTV_BPP 4
#define CTV_SIZE 32
#define CTV_ROWS _
#define CTV_CARE c
#define CTV_FLIPX f
#define CTV_MASK b
#include "ctv_do_template.h"
#undef CTV_BPP
#undef CTV_SIZE
#undef CTV_ROWS
#undef CTV_CARE
#undef CTV_FLIPX
#undef CTV_MASK
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#define CU_ROWS  (1)

#define CU_CARE  (0)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#define CU_CARE  (1)
#define CU_FLIPX (0)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#define CU_FLIPX (1)
// Invalid combination of capabilities. rows 1  size 20  mask 2
#undef  CU_FLIPX
#undef  CU_CARE

#undef  CU_ROWS

#undef  CU_SIZE

#undef  CU_BPP

#undef  CU_MASK



// Filler function
static INT32 CtvDo_______() { return 0; }



// Lookup table for 2 bpp
CtvDoFn CtvDo2[0x20]={
CtvDo28____,CtvDo28__f_,CtvDo28_c__,CtvDo28_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo216____,CtvDo216__f_,CtvDo216_c__,CtvDo216_cf_,
CtvDo216r___,CtvDo216r_f_,CtvDo216rc__,CtvDo216rcf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo232____,CtvDo232__f_,CtvDo232_c__,CtvDo232_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 3 bpp
CtvDoFn CtvDo3[0x20]={
CtvDo38____,CtvDo38__f_,CtvDo38_c__,CtvDo38_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo316____,CtvDo316__f_,CtvDo316_c__,CtvDo316_cf_,
CtvDo316r___,CtvDo316r_f_,CtvDo316rc__,CtvDo316rcf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo332____,CtvDo332__f_,CtvDo332_c__,CtvDo332_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 4 bpp
CtvDoFn CtvDo4[0x20]={
CtvDo48____,CtvDo48__f_,CtvDo48_c__,CtvDo48_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo416____,CtvDo416__f_,CtvDo416_c__,CtvDo416_cf_,
CtvDo416r___,CtvDo416r_f_,CtvDo416rc__,CtvDo416rcf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo432____,CtvDo432__f_,CtvDo432_c__,CtvDo432_cf_,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 2 bpp with Sprite Masking
CtvDoFn CtvDo2m[0x20]={
CtvDo28___m,CtvDo28__fm,CtvDo28_c_m,CtvDo28_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo216___m,CtvDo216__fm,CtvDo216_c_m,CtvDo216_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo232___m,CtvDo232__fm,CtvDo232_c_m,CtvDo232_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 3 bpp with Sprite Masking
CtvDoFn CtvDo3m[0x20]={
CtvDo38___m,CtvDo38__fm,CtvDo38_c_m,CtvDo38_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo316___m,CtvDo316__fm,CtvDo316_c_m,CtvDo316_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo332___m,CtvDo332__fm,CtvDo332_c_m,CtvDo332_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 4 bpp with Sprite Masking
CtvDoFn CtvDo4m[0x20]={
CtvDo48___m,CtvDo48__fm,CtvDo48_c_m,CtvDo48_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo416___m,CtvDo416__fm,CtvDo416_c_m,CtvDo416_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo432___m,CtvDo432__fm,CtvDo432_c_m,CtvDo432_cfm,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 2 bpp with BgHi
CtvDoFn CtvDo2b[0x20]={
CtvDo28___b,CtvDo28__fb,CtvDo28_c_b,CtvDo28_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo216___b,CtvDo216__fb,CtvDo216_c_b,CtvDo216_cfb,
CtvDo216r__b,CtvDo216r_fb,CtvDo216rc_b,CtvDo216rcfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo232___b,CtvDo232__fb,CtvDo232_c_b,CtvDo232_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 3 bpp with BgHi
CtvDoFn CtvDo3b[0x20]={
CtvDo38___b,CtvDo38__fb,CtvDo38_c_b,CtvDo38_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo316___b,CtvDo316__fb,CtvDo316_c_b,CtvDo316_cfb,
CtvDo316r__b,CtvDo316r_fb,CtvDo316rc_b,CtvDo316rcfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo332___b,CtvDo332__fb,CtvDo332_c_b,CtvDo332_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};
// Lookup table for 4 bpp with BgHi
CtvDoFn CtvDo4b[0x20]={
CtvDo48___b,CtvDo48__fb,CtvDo48_c_b,CtvDo48_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo416___b,CtvDo416__fb,CtvDo416_c_b,CtvDo416_cfb,
CtvDo416r__b,CtvDo416r_fb,CtvDo416rc_b,CtvDo416rcfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
CtvDo432___b,CtvDo432__fb,CtvDo432_c_b,CtvDo432_cfb,
CtvDo_______,CtvDo_______,CtvDo_______,CtvDo_______,
};


// Current BPP:
CtvDoFn CtvDoX[0x20];
CtvDoFn CtvDoXM[0x20];
CtvDoFn CtvDoXB[0x20];


