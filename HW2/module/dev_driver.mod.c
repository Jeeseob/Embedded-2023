#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x8a2e525e, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x45a55ec8, "__iounmap" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xec95baea, "__register_chrdev" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0xf9a482f9, "msleep" },
	{ 0x5f754e5a, "memset" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xd627480b, "strncat" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x27e1a049, "printk" },
	{ 0xa170bbdb, "outer_cache" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

