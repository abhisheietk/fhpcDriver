#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v000010B5d00009656sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000156sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001556d00001100sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000188sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000153sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000144sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000188sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001679d00000001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001679d00000005sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd00006014sv*sd*bc*sc*i*");
