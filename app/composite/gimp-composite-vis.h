#ifndef gimp_composite_vis_h
#define gimp_composite_vis_h

extern void gimp_composite_vis_init (void);

/*
	* The function gimp_composite_*_install() is defined in the code generated by make-install.py
	* I hate to create a .h file just for that declaration, so I do it here (for now).
 */
extern void gimp_composite_vis_install (void);

#endif
