#!/usr/bin/perl

# sent to me by Seth Burgess <sjburges@gimp.org>
# small changes my Marc Lehmann <pcg@goof.com>

use Gimp;
use Gimp::Fu;

#Gimp::set_trace(TRACE_CALL);

sub windify {
	my ($img, $drawable, $angle, $density, $distance, $wrap) = @_;
	my $oldbg = gimp_palette_get_background();
	my $xsize = gimp_drawable_width($drawable);
	my $ysize = gimp_drawable_height($drawable);

	my $out = gimp_image_new($xsize,$ysize,0);
	gimp_palette_set_background([128,128,128]);
	my $windlayer = gimp_layer_new($out,$xsize,$ysize,RGB_IMAGE,"Windlayer",100,NORMAL_MODE);
	gimp_drawable_fill($windlayer, 0);
	gimp_image_add_layer($out,$windlayer,0);
	my $windlayercopy = gimp_layer_copy($windlayer, 1);
	gimp_image_add_layer($out,$windlayercopy,0);
	plug_in_noisify(1,$out,$windlayercopy,0,$density/255,
                                                $density/255,
                                                $density/255,1);

	plug_in_mblur(1,$out,$windlayercopy,0,15,$angle);
	gimp_layer_set_mode($windlayercopy, 10); # Lighten Only
	gimp_image_merge_visible_layers($out,0);

# many thanks to Dov for this suggestion as a workaround to the 
# gimp_image_merge_visible_layers bug

	my $newlay = gimp_image_get_active_layer ($out);
	plug_in_displace(1,$img,$drawable,-$distance*cos($angle*180/3.14159),
                                          $distance*sin($angle*180/3.14159),
					  1,1, $newlay,$newlay, $wrap);
	gimp_image_remove_layer($out,$newlay);
	gimp_image_delete ($out);
	gimp_palette_set_background($oldbg);
	gimp_displays_flush();
	
	undef;
	}

register
	"windify",
	"Add wind to an image",
	"Blow your image all over :)",
	"Seth Burgess",
	"Seth Burgess <sjburges\@gimp.org>",
	"1998-09-14",
	"<Image>/Filters/Distorts/Windify",
	"*",
	[
	 [PF_INT32, "angle", "Wind Angle, 0 is left", 120],
	 [PF_INT32, "density", "How Much Is Blown",80],
	 [PF_VALUE, "distance", "How Far Its Blown",30],
	 [PF_TOGGLE, "smear", "Smear on Edges (or Wrap)",0]
	],
	\&windify;

exit main;

