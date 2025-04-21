/*
 * Copyright (c) 2021, 2022, BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INIT_INTRINFO_H
#define __INIT_INTRINFO_H

/* legacy PCI INTPIN interrupts */
extern struct callout_rtn interrupt_id_j7_pci_intpin;
extern struct callout_rtn interrupt_eoi_j7_pci_intpin;
extern struct callout_rtn interrupt_mask_j7_pci_intpin;
extern struct callout_rtn interrupt_unmask_j7_pci_intpin;
#endif /* __INIT_INTRINFO_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/hardware/branches/release/hardware/startup/boards/ti-j7/init_intrinfo.h $ $Rev: 989189 $")
#endif
