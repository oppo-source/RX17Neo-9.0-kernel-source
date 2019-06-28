/* Copyright (c) 2014-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/kmemleak.h>
#include <linux/async.h>
#include <linux/thread_info.h>
#include <soc/qcom/memory_dump.h>
#include <soc/qcom/minidump.h>
#include <asm/sections.h>

#ifdef VENDOR_EDIT //Fanhong.Kong@PSW.BSP.CHG,add 2017/10/10 for O mini dump
#include <linux/uaccess.h>
#include <asm-generic/irq_regs.h>
#include <linux/irq.h>
#include <linux/percpu.h>
#endif/*VENDOR_EDIT*/

#define MISC_DUMP_DATA_LEN		4096
#define PMIC_DUMP_DATA_LEN		(64 * 1024)
#define VSENSE_DUMP_DATA_LEN		4096
#define RPM_DUMP_DATA_LEN		(160 * 1024)

void register_misc_dump(void)
{
	void *misc_buf;
	int ret;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *misc_data;

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) > 1) {
		misc_data = kzalloc(sizeof(struct msm_dump_data), GFP_KERNEL);
		if (!misc_data)
			return;
		misc_buf = kzalloc(MISC_DUMP_DATA_LEN, GFP_KERNEL);
		if (!misc_buf)
			goto err0;

		strlcpy(misc_data->name, "KMISC", sizeof(misc_data->name));
		misc_data->addr = virt_to_phys(misc_buf);
		misc_data->len = MISC_DUMP_DATA_LEN;
		dump_entry.id = MSM_DUMP_DATA_MISC;
		dump_entry.addr = virt_to_phys(misc_data);
		ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS, &dump_entry);
		if (ret)
			goto err1;
		return;
err1:
		kfree(misc_buf);
err0:
		kfree(misc_data);
	}
}

static void register_pmic_dump(void)
{
	static void *dump_addr;
	int ret;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *dump_data;

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) > 1) {
		dump_data = kzalloc(sizeof(struct msm_dump_data), GFP_KERNEL);
		if (!dump_data) {
			pr_err("dump data structure allocation failed\n");
			return;
		}
		dump_addr = kzalloc(PMIC_DUMP_DATA_LEN, GFP_KERNEL);
		if (!dump_addr)
			goto err0;

		strlcpy(dump_data->name, "KPMIC", sizeof(dump_data->name));
		dump_data->addr = virt_to_phys(dump_addr);
		dump_data->len = PMIC_DUMP_DATA_LEN;
		dump_entry.id = MSM_DUMP_DATA_PMIC;
		dump_entry.addr = virt_to_phys(dump_data);
		ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS, &dump_entry);
		if (ret) {
			pr_err("Registering pmic dump region failed\n");
			goto err1;
		}
		return;
err1:
		kfree(dump_addr);
err0:
		kfree(dump_data);
	}
}

static void register_vsense_dump(void)
{
	static void *dump_addr;
	int ret;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *dump_data;

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) > 1) {
		dump_data = kzalloc(sizeof(struct msm_dump_data), GFP_KERNEL);
		if (!dump_data) {
			pr_err("dump data structure allocation failed for vsense data\n");
			return;
		}
		dump_addr = kzalloc(VSENSE_DUMP_DATA_LEN, GFP_KERNEL);
		if (!dump_addr)
			goto err0;

		strlcpy(dump_data->name, "KVSENSE",
				sizeof(dump_data->name));
		dump_data->addr = virt_to_phys(dump_addr);
		dump_data->len = VSENSE_DUMP_DATA_LEN;
		dump_entry.id = MSM_DUMP_DATA_VSENSE;
		dump_entry.addr = virt_to_phys(dump_data);
		ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS, &dump_entry);
		if (ret) {
			pr_err("Registering vsense dump region failed\n");
			goto err1;
		}
		return;
err1:
		kfree(dump_addr);
err0:
		kfree(dump_data);
	}
}

void register_rpm_dump(void)
{
	static void *dump_addr;
	int ret;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *dump_data;

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) > 1) {
		dump_data = kzalloc(sizeof(struct msm_dump_data), GFP_KERNEL);
		if (!dump_data)
			return;
		dump_addr = kzalloc(RPM_DUMP_DATA_LEN, GFP_KERNEL);
		if (!dump_addr)
			goto err0;

		strlcpy(dump_data->name, "KRPM", sizeof(dump_data->name));
		dump_data->addr = virt_to_phys(dump_addr);
		dump_data->len = RPM_DUMP_DATA_LEN;
		dump_entry.id = MSM_DUMP_DATA_RPM;
		dump_entry.addr = virt_to_phys(dump_data);
		ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS, &dump_entry);
		if (ret) {
			pr_err("Registering rpm dump region failed\n");
			goto err1;
		}
		return;
err1:
		kfree(dump_addr);
err0:
		kfree(dump_data);
	}
}

static void __init common_log_register_log_buf(void)
{
	char **log_bufp;
	uint32_t *log_buf_lenp;
	uint32_t *fist_idxp;
	struct msm_client_dump dump_log_buf, dump_first_idx;
	struct msm_dump_entry entry_log_buf, entry_first_idx;
	struct msm_dump_data *dump_data;

	log_bufp = (char **)kallsyms_lookup_name("log_buf");
	log_buf_lenp = (uint32_t *)kallsyms_lookup_name("log_buf_len");
	if (!log_bufp || !log_buf_lenp) {
		pr_err("Unable to find log_buf by kallsyms!\n");
		return;
	}
	fist_idxp = (uint32_t *)kallsyms_lookup_name("log_first_idx");
	if (MSM_DUMP_MAJOR(msm_dump_table_version()) == 1) {
		dump_log_buf.id = MSM_LOG_BUF;
		dump_log_buf.start_addr = virt_to_phys(*log_bufp);
		dump_log_buf.end_addr = virt_to_phys(*log_bufp + *log_buf_lenp);
		if (msm_dump_tbl_register(&dump_log_buf))
			pr_err("Unable to register %d.\n", dump_log_buf.id);
		dump_first_idx.id = MSM_LOG_BUF_FIRST_IDX;
		if (fist_idxp) {
			dump_first_idx.start_addr = virt_to_phys(fist_idxp);
			if (msm_dump_tbl_register(&dump_first_idx))
				pr_err("Unable to register %d.\n",
							dump_first_idx.id);
		}
	} else {
		dump_data = kzalloc(sizeof(struct msm_dump_data),
						GFP_KERNEL);
		if (!dump_data)
			return;
		dump_data->len = *log_buf_lenp;
		dump_data->addr = virt_to_phys(*log_bufp);
		entry_log_buf.id = MSM_DUMP_DATA_LOG_BUF;
		entry_log_buf.addr = virt_to_phys(dump_data);
		if (msm_dump_data_register(MSM_DUMP_TABLE_APPS,
							&entry_log_buf)) {
			kfree(dump_data);
			pr_err("Unable to register %d.\n", entry_log_buf.id);
		} else
			kmemleak_not_leak(dump_data);

		if (fist_idxp) {
			dump_data = kzalloc(sizeof(struct msm_dump_data),
							GFP_KERNEL);
			if (!dump_data)
				return;
			dump_data->addr = virt_to_phys(fist_idxp);
			entry_first_idx.id = MSM_DUMP_DATA_LOG_BUF_FIRST_IDX;
			entry_first_idx.addr = virt_to_phys(dump_data);
			if (msm_dump_data_register(MSM_DUMP_TABLE_APPS,
						&entry_first_idx)) {
				kfree(dump_data);
				pr_err("Unable to register %d.\n",
						entry_first_idx.id);
			} else
				kmemleak_not_leak(dump_data);
		}
	}
}

static void __init register_kernel_sections(void)
{
	struct md_region ksec_entry;
	char *data_name = "KDATABSS";
	const size_t static_size = __per_cpu_end - __per_cpu_start;
	void __percpu *base = (void __percpu *)__per_cpu_start;
	unsigned int cpu;

	strlcpy(ksec_entry.name, data_name, sizeof(ksec_entry.name));
	ksec_entry.virt_addr = (uintptr_t)_sdata;
	ksec_entry.phys_addr = virt_to_phys(_sdata);
	ksec_entry.size = roundup((__bss_stop - _sdata), 4);
	if (msm_minidump_add_region(&ksec_entry))
		pr_err("Failed to add data section in Minidump\n");

	/* Add percpu static sections */
	for_each_possible_cpu(cpu) {
		void *start = per_cpu_ptr(base, cpu);

		memset(&ksec_entry, 0, sizeof(ksec_entry));
		scnprintf(ksec_entry.name, sizeof(ksec_entry.name),
			"KSPERCPU%d", cpu);
		ksec_entry.virt_addr = (uintptr_t)start;
		ksec_entry.phys_addr = per_cpu_ptr_to_phys(start);
		ksec_entry.size = static_size;
		if (msm_minidump_add_region(&ksec_entry))
			pr_err("Failed to add percpu sections in Minidump\n");
	}
}

#ifdef VENDOR_EDIT //yixue.ge@bsp.drv add for dump cpu contex for minidump
#define CPUCTX_VERSION 1
#define CPUCTX_MAIGC1 0x4D494E49
#define CPUCTX_MAIGC2 (CPUCTX_MAIGC1 + CPUCTX_VERSION)

struct cpudatas{
	struct pt_regs 			pt;
	unsigned int 			regs[32][512];//X0-X30 pc
	unsigned int 			sps[1024];
	unsigned int 			ti[16];//thread_info
	unsigned int			task_struct[1024];
};//16 byte alignment

struct cpuctx{
	unsigned int magic_nubmer1;
	unsigned int magic_nubmer2;
	unsigned int dump_cpus;
	unsigned int reserve;
	struct cpudatas datas[0];
};

static struct cpuctx *Cpucontex_buf = NULL;

extern int oops_count(void);
extern int panic_count(void);

extern struct pt_regs * get_arm64_cpuregs(struct pt_regs *regs);

void dumpcpuregs(struct pt_regs *pt_regs)
{
	unsigned int cpu = smp_processor_id();
	struct cpudatas *cpudata = NULL;
	struct pt_regs *regs = pt_regs;
	struct pt_regs regtmp;
	u32	*p;
	unsigned long addr;
	mm_segment_t fs;
	int i,j;

	if(Cpucontex_buf == NULL)
		return;

	if(oops_count() >= 1 && panic_count() >= 1)
		return;

	cpudata = &Cpucontex_buf->datas[cpu];

	if(regs != NULL && user_mode(regs)){ //at user mode we must clear pt struct
		//clear flag
		Cpucontex_buf->dump_cpus &=~(0x01 << cpu);
		return;
	}

	if(regs == NULL){
		regs = get_irq_regs();
		if(regs == NULL){
			memset((void*)&regtmp,0,sizeof(struct pt_regs));
			get_arm64_cpuregs(&regtmp);
			regs = &regtmp;
		}
	}

	//set flag
	Cpucontex_buf->dump_cpus |= (0x01 << cpu);

	fs = get_fs();
	set_fs(KERNEL_DS);
	//1.fill pt
	memcpy((void*)&cpudata->pt,(void*)regs,sizeof(struct pt_regs));;
	//2.fill regs
	//2.1 fill x0-x30
	for(i = 0; i < 31; i++){
		addr = regs->regs[i];
		if (!virt_addr_valid(addr) || addr < KIMAGE_VADDR || addr > -256UL)
			continue;
		addr = addr - 256*sizeof(int);
		p = (u32 *)((addr) & ~(sizeof(u32) - 1));
		addr = (unsigned long)p;
		cpudata->regs[i][0] = (unsigned int)(addr&0xffffffff);
		cpudata->regs[i][1] = (unsigned int)((addr>>32)&0xffffffff);
		for(j = 2;j < 512;j++){
			u32	data;
			if (probe_kernel_address(p, data)) {
				break;
			}else{
				cpudata->regs[i][j] = data;
			}
			++p;
			}
	}
	//2.2 fill pc
	addr = regs->pc;
	if (virt_addr_valid(addr) && addr >= KIMAGE_VADDR 
		&& addr < -256UL){
		addr = addr - 256*sizeof(int);
		p = (u32 *)((addr) & ~(sizeof(u32) - 1));
		addr = (unsigned long)p;
		cpudata->regs[i][0] = (unsigned int)(addr&0xffffffff);
		cpudata->regs[i][1] = (unsigned int)((addr>>32)&0xffffffff);
		for(j = 2;j < 512;j++){
			u32	data;
			if (probe_kernel_address(p, data)) {
				break;
			}else{
				cpudata->regs[31][j] = data;
			}
			++p;
		}
	}
	//3. fill sp
	addr = regs->sp;
	if (virt_addr_valid(addr) && addr >= KIMAGE_VADDR && addr < -256UL){
		addr = addr - 512*sizeof(int);
		p = (u32 *)((addr) & ~(sizeof(u32) - 1));
		addr = (unsigned long)p;
		cpudata->sps[0] = (unsigned int)(addr&0xffffffff);
		cpudata->sps[1] = (unsigned int)((addr>>32)&0xffffffff);
		for(j = 2;j < 1024;j++){
			u32	data;
			if (probe_kernel_address(p, data)) {
				break;
			}else{
				cpudata->sps[j] = data;
			}
			++p;
		}
	}
	//4. fill task_strcut thread_info
	addr = (unsigned long)current;
	if(virt_addr_valid(addr) && addr >= KIMAGE_VADDR && addr < -256UL){
		cpudata->task_struct[0] = (unsigned int)(addr&0xffffffff);
		cpudata->task_struct[1] = (unsigned int)((addr>>32)&0xffffffff);
		memcpy(&cpudata->task_struct[2],(void*)current,sizeof(struct task_struct));
		addr = (unsigned long)(current->stack);
		if(virt_addr_valid(addr)&& addr >= KIMAGE_VADDR && addr < -256UL){
			cpudata->ti[0] = (unsigned int)(addr&0xffffffff);
			cpudata->ti[1] = (unsigned int)((addr>>32)&0xffffffff);
			memcpy(&cpudata->ti[2],(void*)addr,sizeof(struct thread_info));
		}
	}
	set_fs(fs);
}
EXPORT_SYMBOL(dumpcpuregs);

static void __init register_cpu_contex(void)
{
	int ret;
	struct msm_dump_entry dump_entry;
	struct msm_dump_data *dump_data;

	if (MSM_DUMP_MAJOR(msm_dump_table_version()) > 1) {
		dump_data = kzalloc(sizeof(struct msm_dump_data), GFP_KERNEL);
		if (!dump_data)
			return;
		Cpucontex_buf = ( struct cpuctx *)kzalloc(sizeof(struct cpuctx) + 
		 		sizeof(struct cpudatas)* num_present_cpus(),GFP_KERNEL);

		if (!Cpucontex_buf)
			goto err0;
		//init magic number
		Cpucontex_buf->magic_nubmer1 = CPUCTX_MAIGC1;
		Cpucontex_buf->magic_nubmer2 = CPUCTX_MAIGC2;//version
		Cpucontex_buf->dump_cpus = 0;//version

		strlcpy(dump_data->name, "cpucontex", sizeof(dump_data->name));
		dump_data->addr = virt_to_phys(Cpucontex_buf);
		dump_data->len = sizeof(struct cpuctx) + sizeof(struct cpudatas)* num_present_cpus();
		dump_entry.id = 0;
		dump_entry.addr = virt_to_phys(dump_data);
		ret = msm_dump_data_register(MSM_DUMP_TABLE_APPS, &dump_entry);
		if (ret) {
			pr_err("Registering cpu contex dump region failed\n");
			goto err1;
		}
		return;
err1:
		kfree(Cpucontex_buf);
		Cpucontex_buf=NULL;
err0:
		kfree(dump_data);
	}
}
#endif

#ifdef CONFIG_QCOM_MINIDUMP
void dump_stack_minidump(u64 sp)
{
	struct md_region ksp_entry, ktsk_entry;
	u32 cpu = smp_processor_id();

	if (sp < KIMAGE_VADDR || sp > -256UL)
		sp = current_stack_pointer;

	sp &= ~(THREAD_SIZE - 1);
	scnprintf(ksp_entry.name, sizeof(ksp_entry.name), "KSTACK%d", cpu);
	ksp_entry.virt_addr = sp;
	ksp_entry.phys_addr = virt_to_phys((uintptr_t *)sp);
	ksp_entry.size = THREAD_SIZE;
	if (msm_minidump_add_region(&ksp_entry))
		pr_err("Failed to add stack of cpu %d in Minidump\n", cpu);

	scnprintf(ktsk_entry.name, sizeof(ktsk_entry.name), "KTASK%d", cpu);
	ktsk_entry.virt_addr = (u64)current;
	ktsk_entry.phys_addr = virt_to_phys((uintptr_t *)current);
	ktsk_entry.size = sizeof(struct task_struct);
	if (msm_minidump_add_region(&ktsk_entry))
		pr_err("Failed to add current task %d in Minidump\n", cpu);
}
#endif

static void __init async_common_log_init(void *data, async_cookie_t cookie)
{
	register_kernel_sections();
	common_log_register_log_buf();
	register_misc_dump();
	register_pmic_dump();
	register_vsense_dump();
	register_rpm_dump();
#ifdef VENDOR_EDIT //yixue.ge@bsp.drv add for dump cpu contex for minidump
	register_cpu_contex();
#endif
	
}

static int __init msm_common_log_init(void)
{
	/* Initialize asynchronously to reduce boot time */
	async_schedule(async_common_log_init, NULL);
	return 0;
}
late_initcall(msm_common_log_init);
