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

u64* addr;

u64 *addr_s_1;
u64 *addr_s_2;

int __init get_zeroed_page_init(void)
{
    addr = (u64 *)get_zeroed_page(GFP_KERNEL);

    addr[0] = 0x12;
    addr[0x8 / 8] = 0x34;

    addr[0xFFF / 8] = 0x1234;  // a index = offset 64bit

    printk(KERN_INFO "addr va = %lx, addr pa = %lx", (u64)addr, virt_to_phys(addr));
    printk(KERN_INFO "addr[0] = %lx", addr[0]);
    printk(KERN_INFO "addr[0x8 / 8] = %lx", addr[0x8 / 8]);
    printk(KERN_INFO "addr[0xFFF / 8] = %lx", addr[0xFFF / 8]);

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
    printk(KERN_INFO "end get addr");
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
    free_pages((unsigned long)addr ,0);
    // free_pages((unsigned long)addr_s_1 ,10);

    printk(KERN_INFO "exit! \n");
}

module_init(get_zeroed_page_init);
module_exit(get_zeroed_page_exit);
