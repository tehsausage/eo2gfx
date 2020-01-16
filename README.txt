Building:
	Dependencies:
		libpng

	Steps:
		$ cmake .
		$ make

Setup:
	Step 1:
		$ mkdir atlas2 atlas2/gfx atlas2/gfxdupe dump atlastmp

	Step 2:
		Make a sub-folder called "gfx" which contains all EO EGF
		files.

Usage:
	Dependencies:
		PHP
		imagemagick
		pngcrush

	Step 1:
		$ ./gfx_dumper.php
		Dumps EGF files from ./gfx to ./dump, saving each
		  image to a separate PNG file:
			./dump/gfx###.egf/####.png

		Approx. 15.5k files created.

	Step 2:
		$ ./atlas_maker.php:
		Takes dumped files from ./gfx and packs them in to vertically
		  aligned atlasses as specified by the hard-coded rules (starting
		  at approx line 300) and dumps packed images to:
		  	./atlas2/gfx/################.png  (#s represent an 8 byte
		  	                                   hash of the image content)
		  referenced by graphics table files:
		  	./atlas2/gfx###.table
		  which are a binary format describing the positions
		  of the images in each atlas (E2GT format).

		  Exact duplicate image files are dumped to
		  	./atlas2/gfxdupe
		  and can be ignored.

		  Intermediate files are generated in:
		  	./atlastmp
		  and can be ignored.

		  Approx. 7k temp files created.
		  Approx. 2.4k atlas files created (25 tables + ~2.3k images)

	Step 3:
		$ ./fix_alpha.php
		Fixes the transparency of a few atlas images for some reason.
		  Probably important.
		  Hard-coded list of atlas image hashes used.

E2GT Format:
	All multi-byte numbers are big endian I think maybe

	Header:
		[4] Magic bytes      ("E2GT" ASCII)
		[2] Version number   (0x0001)
		[2] Number of images

	Body:
		( repeated for every image from 1 .. Number of images inclusive )
		{
			[8] Hash code corresponding to the gfx image filename
				(e.g. 0x123456789ABCDEF refers to gfx/0123456789abcdef.png)
			[2] Image number
			[2] X offset of image in atlas
			[2] Width of image
			[2] Height of image
		}

E2GTv2 Format:
	Header:
		[4] Magic bytes:    ("E2GT" ASCII)
		[2] Version number  (0x0002)
		[2] Number of hash codes
		[2] Number of images

	Body1:
		( repeated for every unique hash code in the file )
		{
			[8] Hash code corresponding to the gfx image filename
				(e.g. 0x
		}


	Body2:
		( repeated for every image from 1 .. Number of images inclusive )
		{
			[8] Hash code corresponding to the gfx image filename
				(e.g. 0x123456789ABCDEF refers to gfx/0123456789abcdef.png)
			[2] Image number
			[2] X offset of image in atlas
			[2] Width of image
			[2] Height of image
			[1] X adjust of image
			[1] Y adjust of image
			[1] X extend of image
			[1] Y extend of image
		}


E2SS Format:
	Header:
		[4] Magic bytes:    ("E2SS" ASCII)
		[2] Version number  (0x0001)
		[2] Number of sprite sheets

		Body:
			( repeated for each sprite sheet )
			{
				[2] Number of graphics
				( repeated for each graphic in sheet)
				{
					[2] Y offset
				}
			}

