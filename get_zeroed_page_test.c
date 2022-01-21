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

u64 *addr_p4d;
u64 **addr_pud;

u64 *addr_s_1;
u64 *addr_s_2;

#define INDEX_END 0xFF8

int __init get_zeroed_page_init(void)
{
    printk(KERN_ERR "enter get_zeroed_page_init");
    printk("This is hello_module, welcome to Linux kernel \n create all page");
    printk(KERN_INFO "__START_KERNEL_map = %llx, PAGE_OFFSET = %llx", __START_KERNEL_map, PAGE_OFFSET);

    addr_p4d = (u64 *)get_zeroed_page(GFP_KERNEL);

    /*
    addr_p4d[0] = 0x12;
    addr_p4d[0x8 / 8] = 0x34;

    addr_p4d[0xFFF / 8] = 0x1234;  // a index = offset 64bit

    printk(KERN_INFO "addr_p4d va = %lx, addr_p4d pa = %lx", (u64)addr_p4d, virt_to_phys(addr_p4d));
    printk(KERN_INFO "addr_p4d[0] = %lx", addr_p4d[0]);
    printk(KERN_INFO "addr_p4d[0x8 / 8] = %lx", addr_p4d[0x8 / 8]);
    printk(KERN_INFO "addr_p4d[0xFFF / 8] = %lx", addr_p4d[0xFFF / 8]);
    */
    addr_pud = (u64 **)get_zeroed_page(GFP_KERNEL);
    addr_p4d[0x888 / 8] = virt_to_phys(addr_pud) + 0x67;

    printk(KERN_INFO "addr_p4d va = %lx, addr_p4d pa = %lx", (u64)addr_p4d, virt_to_phys(addr_p4d));

    printk(KERN_INFO "addr_pud va = %lx, addr_pud pa = %lx", (u64)addr_pud, virt_to_phys(addr_pud));
    printk(KERN_INFO "addr_p4d[0x888 / 8] = %lx", addr_p4d[0x888 / 8]);

    u32 i;
    u32 j;
    for (i = 0; i<= INDEX_END / 8; i++)
    {
        addr_pud[i] = (u64 *)get_zeroed_page(GFP_KERNEL);
    }

    for (i = 0; i<= INDEX_END / 8; i++)
    {
        for (j = 0; j<= INDEX_END / 8; j++)
        {
            addr_pud[i][j] = 0x888000000000 +(((i * 8) >> 3) << 30) +(((j * 8) >> 3) << 21) -  0x888000000000 + 0xE7;
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
        addr_pud[i] = virt_to_phys(addr_pud[i]) + 0x67;
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
    u32 i;
    for (i = 0; i<= INDEX_END / 8; i++)
    {
        // free_pages((unsigned long)addr_pud[i], 0);  // free_pages(...,0) equals free_page(...)
        free_page((unsigned long)addr_pud[i] + PAGE_OFFSET - 0x67);
    }

    free_page((unsigned long)addr_pud);
    free_page((unsigned long)addr_p4d);
    // free_pages((unsigned long)addr_s_1 ,10);

    printk(KERN_ERR "exit! \n");
}

module_init(get_zeroed_page_init);
module_exit(get_zeroed_page_exit);
