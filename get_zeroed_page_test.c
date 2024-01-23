/*************************************************************************
	> File Name: get_zeroed_page_test.c
	> Author:
	> Mail:
	> Created Time: Friday, January 21, 2022 AM12:57:07 HKT
 ************************************************************************/

#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/dmar.h>
#include <linux/intel-iommu.h>
#include <linux/iommu.h>
#include <linux/memory.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/spinlock.h>

#include <linux/intel-svm.h>

MODULE_LICENSE("GPL");
static int __init get_zeroed_page_init(void);
static void __exit get_zeroed_page_exit(void);

/*
u64 *addr_p4d;
u64 ***addr_pud;

u64 *addr_s_1;
u64 *addr_s_2;

u64 addr_temp_i[512];
u64 addr_temp_ij[512][512];
*/

struct page_from_get_zero
{
        u64 *addr_p4d;
        u64 ***addr_pud;

        u64 addr_temp_i[512];
        u64 addr_temp_ij[512][512];
}page;

struct sm_table_get_zero
{
        u64 *sm_root_t;
        u64 *sm_context_t;
        u64 *sm_pasid_dir_t;
        u64 *sm_pasid_t;
}sm_table;

static int bus = 0x0;
static int dev = 0x0;
static int func = 0x0;
module_param(bus, int, S_IRUSR);
module_param(dev, int, S_IRUSR);
module_param(func, int, S_IRUSR);

#define INDEX_END  0xFF8

#define BUS_NUM  (bus)// 0x86
#define DEV_NUM  (dev)// 0x0  // dev 0-15 use lower context table, dev 16-31 use upper context table
// #define DEV_NUM  12
#define FUNC_NUM  (func)// 0

#define ROOT_UP  0x1
#define ROOT_LP  0x1

// #define CONTEXT_P  0x1
// #define CONTEXT_DTE  0x4
// #define CONTEXT_PASIDE  0x8
// #define CONTEXT_PDTS  (1 << 9)
#define PASID_CONTEXT_P  0X1
#define PASID_CONTEXT_DTE_SET  (1 << 2)
#define PASID_CONTEXT_PASIDE  (1 << 3)
#define PASID_CONTEXT_PRE_SET  (1 << 4)
#define PASID_CONTEXT_PDTS_256  (1 << 9)
#define PASID_CONTEXT_RID_PASID_RESERVED  (0 << (64-64))  // request without PASID will use RID_PASID val to walk PASID table
#define PASID_CONTEXT_RID_PRIV_RESERVED  (0 << (84-64))

#define PASID_CONTEXT_RID_PASID_Set(val)  (val << (64-64))  // test RID_PASID=323
#define PASID_CONTEXT_RID_PRIV_Set  (1 << (84-64))

#define PASID_DIR_P  0x1
#define PASID_DIR_FPD_RESERVED  (0 << 1)


#define PASID_P  0x1
#define PASID_AW  (2 << 2)
#define PASID_SLEE_PREVENTED  (1 << 5)
#define PASID_PGTT_FL_ONLY  (1 << 6)
#define PASID_PGTT_SL_ONLY  (1 << 7)
#define PASID_SLADE  (1 << 9)

#define PASID_DID  (0x18 << (64-64))
#define PASID_PWSNP  (1 << (87-64))
#define PASID_PGSNP  (1 << (88-64))
#define PASID_CD_ENABLE  (0 << (89-64))
#define PASID_EMTE_IGNORED  (0 << (90-64))
#define PASID_EMT_RESERVED  (0 << (91-64))
#define PASID_PWT_RESERVED  (0 << (94-64))
#define PASID_PCD_RESERVED  (0 << (95-64))

#define PASID_SRE_RESERVED (0 << (128-128))
#define PASID_ERE_RESERVED  (1 << (129-128))
#define PASID_FLPM_4L  (0 << (130-128))
#define PASID_WPE_RESERVED  (0 << (132-128))
#define PASID_NXE_RESERVED  (0 << (133-128))
#define PASID_SMPE_RESERVED  (0 << (134-128))
#define PASID_EAFE_SET  (1 << (135-128))


static int pasid = 0x0;
static int rid_pasid = 0x0;
module_param(pasid, int, S_IRUSR);
module_param(rid_pasid, int, S_IRUSR);

static int pgtt = 0x0;
module_param(pgtt, int, S_IRUSR);

// int pasid_val = 257;
#define pasid_val (pasid)// int pasid_val = 323;

#define rid_pasid_val (rid_pasid)// int rid_pasid_val = 323;
// int rid_pasid_val = 0;

int __init get_sm_table(int need_sm_t, u64 addr_p4d_val)
{
        if (!need_sm_t)
                return -1;

        pr_info("*******************************************************************");
        pr_info("*******************************************************************");
        printk(KERN_ERR "enter get_sm_table_init\n");
        printk("This module will create sm table\n");
        printk(KERN_INFO "__START_KERNEL_map = 0x%llx, PAGE_OFFSET = 0x%llx\n",
               __START_KERNEL_map, PAGE_OFFSET);

        sm_table.sm_root_t = (u64 *)get_zeroed_page(GFP_KERNEL);
        sm_table.sm_context_t = (u64 *)get_zeroed_page(GFP_KERNEL);
        sm_table.sm_pasid_dir_t = (u64 *)get_zeroed_page(GFP_KERNEL);
        sm_table.sm_pasid_t = (u64 *)get_zeroed_page(GFP_KERNEL);

        printk(KERN_INFO "sm_root_t va = 0x%lx, sm_root_t pa = 0x%lx\n",  // Don't set the sm_root_t pa into RTADDR_REG!!
              (u64)(sm_table.sm_root_t), virt_to_phys(sm_table.sm_root_t));  // be sure the TTM=01b(scalable mode) into RTADDR_REG!!

        printk(KERN_INFO "sm_context_t va = 0x%lx, sm_context_t pa = 0x%lx\n",  // set the sm_context_t pa into SM Root Table entry!!
              (u64)(sm_table.sm_context_t), virt_to_phys(sm_table.sm_context_t));

        printk(KERN_INFO "sm_pasid_dir_t va = 0x%lx, sm_pasid_dir_t pa = 0x%lx\n",
              (u64)(sm_table.sm_pasid_dir_t), virt_to_phys(sm_table.sm_pasid_dir_t));

        printk(KERN_INFO "sm_pasid_t va = 0x%lx, sm_pasid_t pa = 0x%lx\n",
              (u64)(sm_table.sm_pasid_t), virt_to_phys(sm_table.sm_pasid_t));

        pr_info("*******************************************************************");
        pr_info("BUS_NUM = %#X, DEV_NUM = %#x, FUNC_NUM = %#x\n",BUS_NUM, DEV_NUM, FUNC_NUM);

        if (DEV_NUM <= 0xF && DEV_NUM >= 0)  // index val 1 means 64bit, offset 0x10, one sm_root_t entry 128bit
        {
                pr_info("DEV_NUM <= 0xF(15) Using SM Lower Table\n");
                pr_info("sm_root_t 128bit entry index -- BUS_NUM * 2 = %d, offset=index*8 = %#x\n",
                        BUS_NUM * 2, BUS_NUM * 2 * 8);
                sm_table.sm_root_t[BUS_NUM * 2] = virt_to_phys(sm_table.sm_context_t) | ROOT_LP;  // SM Root entry UCTP

                int offset_sm_context = DEV_NUM << 8 | FUNC_NUM << 5;
                pr_info("sm_context_t DEV_NUM << 8 | FUNC_NUM << 5 offset_sm_context = %#x\n",
                        offset_sm_context);
                pr_info("sm_context_t 256bit entry index -- offset_sm_context / 8 = %d(%#x)\n",
                        offset_sm_context / 8, offset_sm_context / 8);

                if (rid_pasid_val != 0)
                {
                        pr_info("set rid_pasid in sm_context_t\n");
                        sm_table.sm_context_t[offset_sm_context / 8] = virt_to_phys(sm_table.sm_pasid_dir_t) | PASID_CONTEXT_P |
                                                                        PASID_CONTEXT_PASIDE | PASID_CONTEXT_DTE_SET |
                                                                        PASID_CONTEXT_PRE_SET | PASID_CONTEXT_PDTS_256;
                        sm_table.sm_context_t[offset_sm_context / 8 + 1] |= PASID_CONTEXT_RID_PASID_Set(rid_pasid_val) |
                                                                        PASID_CONTEXT_RID_PRIV_RESERVED;  // for test RID_PASID
                }
                else if (rid_pasid_val == 0)
                {
                        sm_table.sm_context_t[offset_sm_context / 8] = virt_to_phys(sm_table.sm_pasid_dir_t) | PASID_CONTEXT_P |
                                                                        PASID_CONTEXT_PASIDE | PASID_CONTEXT_DTE_SET |
                                                                        PASID_CONTEXT_PRE_SET | PASID_CONTEXT_PDTS_256;
                }

                // sm_context_t entry 256bit
                // dev 0x0 func 0x0, offset 0x000, index 0
                // dev 0x0 func 0x1, offset 0x020, index 4
                // dev 0x0 func 0x6, offset 0x0c0, index 24
                // ... etc
                // dev 0x0 func 0x7, offset 0x0e0, index 28
                // ... etc
                // dev 0x1 func 0x7, offset 0x1e0, index 60
                // dev 0x2 func 0x7, offset 0x2e0, index 92
                // ... etc
                // dev 0xf func 0x7, offset 0xfe0, index 508
                // offset = dev << 8 | func << 5, index = offset / 8
        }
        else if (DEV_NUM <=0xFF && DEV_NUM >= 0x10)
        {
                pr_info("DEV_NUM >= 0x10(16) Using SM Upper Context Table\n");
                pr_info("sm_root_t 128bit entry index -- BUS_NUM * 2 + 1 = %d, offset=index*8 = %#x\n",
                        BUS_NUM * 2 + 1, (BUS_NUM * 2 + 1) * 8);
                sm_table.sm_root_t[BUS_NUM * 2 + 1] = virt_to_phys(sm_table.sm_context_t) | ROOT_UP;  // SM Root entry LCTP

                int offset_sm_context = (DEV_NUM - 0x10) << 8 | FUNC_NUM << 5;  // 16 equals 0, 31 equals 15
                pr_info("sm_context_t DEV_NUM << 8 | FUNC_NUM << 5 offset_sm_context = %#x\n",
                        offset_sm_context);
                pr_info("sm_context_t 256bit entry index -- offset_sm_context / 8 = %d(%#x)\n",
                        offset_sm_context / 8, offset_sm_context / 8);

                if (rid_pasid_val != 0)
                {
                        pr_info("set rid_pasid in sm_context_t\n");
                        sm_table.sm_context_t[offset_sm_context / 8] = virt_to_phys(sm_table.sm_pasid_dir_t) | PASID_CONTEXT_P |
                                                                        PASID_CONTEXT_PASIDE | PASID_CONTEXT_DTE_SET |
                                                                        PASID_CONTEXT_PRE_SET | PASID_CONTEXT_PDTS_256;
                        sm_table.sm_context_t[offset_sm_context / 8 + 1] |= PASID_CONTEXT_RID_PASID_Set(rid_pasid_val) |
                                                                        PASID_CONTEXT_RID_PRIV_RESERVED;
                }
                else if (rid_pasid_val == 0)
                {
                        sm_table.sm_context_t[offset_sm_context / 8] = virt_to_phys(sm_table.sm_pasid_dir_t) | PASID_CONTEXT_P |
                                                                        PASID_CONTEXT_PASIDE | PASID_CONTEXT_DTE_SET |
                                                                        PASID_CONTEXT_PRE_SET | PASID_CONTEXT_PDTS_256;
                }
        }

        if (rid_pasid_val != 0)
        {
                pr_info("Used RID PASID, pasid_val=rid_pasid_val=%d\n", rid_pasid_val);
                pasid_val = rid_pasid_val;
        }

        pr_info("pasid_val = %d", pasid_val);
        if (pasid_val & 0x100000)
        {
                pr_err("pasid_val above 20bit!!!!\n");
                return -1;
        }

        // index val 2 means 128bit, index val 4 means 256bit

        int pasid_val_0_5 = pasid_val & 0x3F;
        int pasid_val_6_19 = pasid_val >> 6;
        pr_info("pasid_val_6_19 = %d, %#x\n", pasid_val_6_19, pasid_val_6_19);
        if (pasid_val_6_19 >= 0)
        {
                pr_info("sm_pasid_dir_t 64bit index -- pasid_val_6_19 = %d, offset=index*8=%#x\n",
                        pasid_val_6_19, pasid_val_6_19 * 8);
                sm_table.sm_pasid_dir_t[pasid_val_6_19] = virt_to_phys(sm_table.sm_pasid_t) |
                                                          PASID_DIR_P | PASID_DIR_FPD_RESERVED;
                // one sm_pasid_dir_t entry 64bit
        }

        pr_info("pasid_val_0_5 = %d, %#x\n", pasid_val_0_5, pasid_val_0_5);
        if (pasid_val_0_5 >=0)
        {
                pr_info("sm_pasid_t 512bit index -- pasid_val_0_5 * 8 = %d, offset=index*8=%#x\n",
                        pasid_val_0_5 * 8, pasid_val_0_5 * 8 * 8);

                if (pgtt == 1)
                {
                        sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 1 + 1] = addr_p4d_val;  // *8 maybe pass
                        // one sm_pasid_t entry 512bit, 0: 0-63bit, 1: 64-127bit, 2: 128-191bit
                        //                              SLPTR                        FLPTR
                        sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 0] |= (PASID_P | PASID_AW | PASID_SLEE_PREVENTED |
                                                                        PASID_PGTT_FL_ONLY | PASID_SLADE);
                }
                else if (pgtt == 2)
                {
                        sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 0] = addr_p4d_val;  // *8 maybe pass
                        // one sm_pasid_t entry 512bit, 0: 0-63bit, 1: 64-127bit, 2: 128-191bit
                        //                              SLPTR                        FLPTR
                        sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 0] |= (PASID_P | PASID_AW |
                                                                        PASID_PGTT_SL_ONLY | PASID_SLADE);
                }
                else if (pgtt == 0)
                {
                        pr_info("!!not input pgtt!!");
                        return 0;
                }

                sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 1] |= PASID_DID;  // DID 0x18 for test
                //
                sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 1] |= (PASID_PWSNP | PASID_PGSNP |
                                                               PASID_CD_ENABLE | PASID_EMTE_IGNORED |
                                                               PASID_EMT_RESERVED | PASID_PWT_RESERVED |
                                                               PASID_PCD_RESERVED);

                sm_table.sm_pasid_t[pasid_val_0_5 * 8 + 1 + 1] |= (PASID_SRE_RESERVED | PASID_ERE_RESERVED |
                                                                   PASID_FLPM_4L | PASID_WPE_RESERVED |
                                                                   PASID_NXE_RESERVED | PASID_SMPE_RESERVED |
                                                                   PASID_EAFE_SET);
        }

        return 0;
}

int __init get_zeroed_page_init(void)
{
        pr_info("*******************************************************************");
        pr_info("*******************************************************************");
        printk(KERN_ERR "enter get_zeroed_page_init\n");
        printk("This module will create all page\n");
        printk(KERN_INFO "__START_KERNEL_map = 0x%llx, PAGE_OFFSET = 0x%llx\n",
               __START_KERNEL_map, PAGE_OFFSET);

        if (PAGE_OFFSET != 0xffff888000000000)
                printk(KERN_ERR "close the randomize memory layout in kernel menuconfig\n");
        if (PAGE_OFFSET == 0xffff888000000000)
                printk(KERN_INFO "PAGE_OFFSET is ok!\n");

        page.addr_p4d = (u64 *)get_zeroed_page(GFP_KERNEL);

        /*
        addr_p4d[0] = 0x12;
        addr_p4d[0x8 / 8] = 0x34;

        addr_p4d[0xFFF / 8] = 0x1234;  // a index = offset 64bit

        printk(KERN_INFO "addr_p4d va = %lx, addr_p4d pa = %lx", (u64)addr_p4d, virt_to_phys(addr_p4d));
        printk(KERN_INFO "addr_p4d[0] = %lx", addr_p4d[0]);
        printk(KERN_INFO "addr_p4d[0x8 / 8] = %lx", addr_p4d[0x8 / 8]);
        printk(KERN_INFO "addr_p4d[0xFFF / 8] = %lx", addr_p4d[0xFFF / 8]);
        */
        page.addr_pud = (u64 ***)get_zeroed_page(GFP_KERNEL);
        page.addr_p4d[0x888 / 8] = virt_to_phys(page.addr_pud) + 0x67;

        printk(KERN_INFO "addr_p4d va = 0x%lx, addr_p4d pa = 0x%lx\n",
               (u64)(page.addr_p4d), virt_to_phys(page.addr_p4d));

        printk(KERN_INFO "addr_pud va = 0x%lx, addr_pud pa = 0x%lx\n",
               (u64)(page.addr_pud), virt_to_phys(page.addr_pud));

        printk(KERN_INFO "addr_p4d[0x888 / 8] = 0x%lx\n",
               (page.addr_p4d)[0x888 / 8]);

        u64 i, j, k;

        for (i = 0; i<= INDEX_END / 8; i++)
        {
                (page.addr_pud)[i] = (u64 **)get_zeroed_page(GFP_KERNEL);
                (page.addr_temp_i)[i] = (unsigned long)(page.addr_pud[i]);
        }

        for (i = 0; i<= INDEX_END / 8; i++)
        {
                for (j = 0; j<= INDEX_END / 8; j++)
                {
                        // addr_pud[i][j] = 0x888000000000 +(((i * 8) >> 3) << 30)
                        // +(((j * 8) >> 3) << 21) -  0x888000000000 + 0xE7;
                        (page.addr_pud)[i][j] = (u64 *)get_zeroed_page(GFP_KERNEL);
                        (page.addr_temp_ij)[i][j] = (unsigned long)(page.addr_pud)[i][j];
                }
        }

        for (i = 0; i<= INDEX_END / 8; i++)
        {
                for (j = 0; j<= INDEX_END / 8; j++)
                {
                        for (k = 0; k<= INDEX_END / 8; k++)
                        {
                                (page.addr_pud)[i][j][k] =
                                        0x888000000000 + (((i * 8) >> 3) << 30) +
                                        (((j * 8) >> 3) << 21) + (((k * 8) >> 3) << 12)
                                        -  0x888000000000 + 0x67;

                                if (i * 8 == 0x40 && j * 8 == 0xb50 && k * 8 == 0x9c0)
                                {
                                        printk(KERN_INFO "addr_pud[%#lx/8][%#lx/8][%#lx/8]=0x%llx\n",
                                               i * 8, j * 8, k * 8, (page.addr_pud)[i][j][k]);

                                        if ((page.addr_pud)[i][j][k] != 0x22d538067)
                                                printk(KERN_ERR "check your page table value\n");

                                        if ((page.addr_pud)[i][j][k] == 0x22d538067)
                                                printk(KERN_INFO "page table seems ok!\n");
                                }
                        }
                }
        }


        /*
        addr_s_1 = (u64 *)__get_free_pages(GFP_KERNEL, 10);
        printk(KERN_INFO "addr_s_1 va = %lx, addr_s_1 pa = %lx", (u64)addr_s_1, virt_to_phys(addr_s_1));
        printk(KERN_INFO "addr_s_1[0x1000 / 8] va = %lx, addr_s_1[0x1000 / 8] pa = %lx", &addr_s_1[0x1000 / 8], virt_to_phys(&addr_s_1[0x1000 / 8]));
        printk(KERN_INFO "addr_s_1[0x2000 / 8] va = %lx, addr_s_1[0x2000 / 8] pa = %lx", &addr_s_1[0x2000 / 8], virt_to_phys(&addr_s_1[0x2000 / 8]));
        printk(KERN_INFO "addr_s_1[0x3000 / 8] va = %lx, addr_s_1[0x3000 / 8] pa = %lx", &addr_s_1[0x3000 / 8], virt_to_phys(&addr_s_1[0x3000 / 8]));
        */

        /*
        if(addr == NULL )
                printk("get_zeroed_page failed! \n");
        else
        {
                printk("get_zeroed_page successfully! addr = 0x%lx\n", (unsigned long)addr );
                printk("the content of mem_spvm+2 is: %d\n", *(addr+2));
                printk("the content of mem_spvm+500 is: %d\n", *(addr+500));
        }
        */
        for (i = 0; i<= INDEX_END / 8; i++)
        {
                for (j = 0; j<= INDEX_END / 8; j++)
                {
                        (page.addr_pud)[i][j] = (u64 *)(virt_to_phys((page.addr_pud)[i][j]) + 0x67);
                }
        }
        for (i = 0; i<= INDEX_END / 8; i++)
        {
                (page.addr_pud)[i] = (u64 **)(virt_to_phys((page.addr_pud)[i]) + 0x67);
        }

        printk(KERN_INFO "end get addr_p4d\n");

        int need_sm_t = 1;
        if (get_sm_table(need_sm_t, virt_to_phys(page.addr_p4d)));
            printk(KERN_INFO "end get sm table\n");

        return 0;
}

void __exit get_zeroed_page_exit(void)
{
        /*
        if(addr != NULL)
        {
                free_pages((unsigned long)addr , 1);     //由于分配一页，这里也释放一页
                printk("free_pages ok! \n");
        }
        */
        u64 i;
        u64 j;

        // free_page(addr_temp_ij[1][1]);
        // u64 temp_addr;

        for (i = 0; i<= INDEX_END / 8; i++)
        {
                for (j = 0; j<= INDEX_END / 8; j++)
                {
                        // free_page((unsigned long)addr_pud[i][j]);
                        // temp_addr = ((unsigned long)addr_pud[0][0]) + PAGE_OFFSET - 0x67;
                        free_page((page.addr_temp_ij)[i][j]);
                }
        }
        printk(KERN_ERR "free ij\n");

        // temp_addr = 0;
        for (i = 0; i<= INDEX_END / 8; i++)
        {
                // free_pages((unsigned long)addr_pud[i], 0);  // free_pages(...,0) equals free_page(...)
                // free_page((unsigned long)addr_pud[i]);
                // temp_addr = ((unsigned long)addr_pud[0]) + PAGE_OFFSET - 0x67;
                free_page((page.addr_temp_i)[i]);
        }
        printk(KERN_ERR "free i\n");

        free_page((unsigned long)(page.addr_pud));
        free_page((unsigned long)(page.addr_p4d));
        // free_pages((unsigned long)addr_s_1 ,10);

        free_page((unsigned long)(sm_table.sm_root_t));
        free_page((unsigned long)(sm_table.sm_context_t));
        free_page((unsigned long)(sm_table.sm_pasid_dir_t));
        free_page((unsigned long)(sm_table.sm_pasid_t));

        printk(KERN_ERR "exit! \n");
}

module_init(get_zeroed_page_init);
module_exit(get_zeroed_page_exit);
