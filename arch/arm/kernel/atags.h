#if 0//#ifdef CONFIG_ATAGS_PROC//CONFIG_ATAGS_PROC is not set
extern void save_atags(struct tag *tags);
#else
static inline void save_atags(struct tag *tags) { }
#endif

void convert_to_tag_list(struct tag *tags);

#if 1//#ifdef CONFIG_ATAGS//CONFIG_ATAGS=y
const struct machine_desc *setup_machine_tags(phys_addr_t __atags_pointer,
	unsigned int machine_nr);
#else
static inline const struct machine_desc *
setup_machine_tags(phys_addr_t __atags_pointer, unsigned int machine_nr)
{
	early_print("no ATAGS support: can't continue\n");
	while (true);
	unreachable();
}
#endif
