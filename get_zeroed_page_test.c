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

#define INDEX_END 0xFF8

int __init get_zeroed_page_init(void)
{
    printk(KERN_ERR "enter get_zeroed_page_init");
    printk("This is hello_module, welcome to Linux kernel \n create all page");
    printk(KERN_INFO "__START_KERNEL_map = %llx, PAGE_OFFSET = %llx", __START_KERNEL_map, PAGE_OFFSET);

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

    printk(KERN_INFO "addr_p4d va = %lx, addr_p4d pa = %lx", (u64)(page.addr_p4d), virt_to_phys(page.addr_p4d));

    printk(KERN_INFO "addr_pud va = %lx, addr_pud pa = %lx", (u64)(page.addr_pud), virt_to_phys(page.addr_pud));
    printk(KERN_INFO "addr_p4d[0x888 / 8] = %lx", (page.addr_p4d)[0x888 / 8]);

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
            // addr_pud[i][j] = 0x888000000000 +(((i * 8) >> 3) << 30) +(((j * 8) >> 3) << 21) -  0x888000000000 + 0xE7;
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
                (page.addr_pud)[i][j][k] = 0x888000000000 + (((i * 8) >> 3) << 30) + (((j * 8) >> 3) << 21) + (((k * 8) >> 3) << 12) -  0x888000000000 + 0x67;
                if (i == 1 && j == 1 && k == 1)
                    printk(KERN_INFO "addr_pud[1][1][1]=addr_pud[0x8/8][0x8/8][0x8/8]=%lx", (page.addr_pud)[1][1][1]);
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

    printk(KERN_INFO "end get addr_p4d");
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
    printk(KERN_ERR "free ij");

    // temp_addr = 0;
    for (i = 0; i<= INDEX_END / 8; i++)
    {
        // free_pages((unsigned long)addr_pud[i], 0);  // free_pages(...,0) equals free_page(...)
        // free_page((unsigned long)addr_pud[i]);
        // temp_addr = ((unsigned long)addr_pud[0]) + PAGE_OFFSET - 0x67;
        free_page((page.addr_temp_i)[i]);
    }
    printk(KERN_ERR "free i");

    free_page((unsigned long)(page.addr_pud));
    free_page((unsigned long)(page.addr_p4d));
    // free_pages((unsigned long)addr_s_1 ,10);

    printk(KERN_ERR "exit! \n");
}

module_init(get_zeroed_page_init);
module_exit(get_zeroed_page_exit);
