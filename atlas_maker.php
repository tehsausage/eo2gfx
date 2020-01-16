#!/usr/bin/env php
<?php

// Take transparency out of these images (mostly GUI)
$transfix = array(
	"03561109367b7769" => 1,
	"10def4cc9250f2e9" => 1,
	"236f9d93dfe36c05" => 1,
	"266cb426c6a4eb3d" => 1,
	"2c42cf216f692e5e" => 1,
	"2dc3c3eed9e6185d" => 1,
	"379d6431abd0f8c7" => 1,
	"3cb6839eefe6a4c0" => 1,
	"44d6ba5861286dda" => 1,
	"4598779055e5aede" => 1,
	"50f777571f6c75fd" => 1,
	"5a9762a5f6e0b44c" => 1,
	"80d81397a61b5404" => 1,
	"8fdeec6d0190c10d" => 1,
	"993447253c557ff5" => 1,
	"a59599ea580bde3e" => 1,
	"a6d18ac36b1f593d" => 1,
	"be69005292c753b9" => 1,
	"bfefbc72d8b6b7a6" => 1,
	"c457c892cbab5267" => 1,
	"e4c56ab2c3007811" => 1,
	"120b1da05647f9c2" => 1,
	"1b1509392f774a80" => 1,
	"26437c8886909984" => 1,
	"324a7e599b3db195" => 1,
	"3745621c8a062428" => 1,
	"37559ae24d8aff39" => 1,
	"398981d5e436d755" => 1,
	"3a9ea6bd302dfc58" => 1,
	"3c28a3e29c7f031d" => 1,
	"3d161e5be354ddfd" => 1,
	"423d2451d412ca7c" => 1,
	"4ee5c4c2c351c305" => 1,
	"5d08208bae504ae6" => 1,
	"5f246ceb505cd292" => 1,
	"696a37eedbeeeee4" => 1,
	"7ce823390291eb8c" => 1,
	"83e55d763940bc50" => 1,
	"94ac24abf0c548a5" => 1,
	"a6e278156ea310d9" => 1,
	"a95496851762f1ea" => 1,
	"ac0546a1a3722e6e" => 1,
	"afa9691b85818f99" => 1,
	"b970115f076033ad" => 1,
	"ba1d09d4008ab072" => 1,
	"be294294f803f201" => 1,
	"bf84183a2a29abc1" => 1,
	"c375a1eef9398c7a" => 1,
	"c3cd9ed818756cb9" => 1,
	"cab957f311940de1" => 1,
	"cae3431b31f3b244" => 1,
	"ceda191b56fb0271" => 1,
	"d009d644104e78d2" => 1,
	"d73068fb1318862b" => 1,
	"d94e98c874fff835" => 1,
	"e186a3a797a5f8c3" => 1,
	"f8db3cedf5c48d9b" => 1,
	"fbebd6c3f4468450" => 1
);

// 75fc4358c8796055 should also be looked at: Replace with real alpha channel

define('GFXDUMP_PREFIX', './dump/');
define('ATLASTMP_PREFIX', './atlastmp/');
define('ATLAS_PREFIX', './atlas2/');
define('GFX_PREFIX', './atlas2/gfx/');
define('GFXDUPE_PREFIX', './atlas2/gfxdupe/');

$this_time = filemtime(__FILE__);
$eoatlas_time = filemtime("./eoatlas");

function gfxdump_png_filename($filename, $i)
{
	return GFXDUMP_PREFIX . $filename . '/' . (100 + $i) . '.png';
}

function atlastmp_filename($filename, $i)
{
	return ATLASTMP_PREFIX . $filename . '/' . ($i) . '.png';
}

function atlas_filename($filename, $i)
{
	return ATLAS_PREFIX . $filename . '/' . ($i) . '.png';
}

function gfx_filename($i)
{
	return GFX_PREFIX . ($i) . '.png';
}

function gfxdupe_filename($i)
{
	return GFXDUPE_PREFIX . ($i) . '.png';
}

function generate_atlas_sane($egfname, $algo)
{
	global $transfix;

	$tmpdir = dirname(atlastmp_filename($egfname, 0));

	if (!is_dir($tmpdir))
		mkdir($tmpdir);

	$table = array();
	$outputs = array();

	for ($i = 0; ; ++$i)
	{
		$id = $i + 1;

		$fileids = $algo($i);

		if (count($fileids) == 0)
			break;

		$filenames = array_map(function($i) use ($egfname) { return gfxdump_png_filename($egfname, $i); }, $fileids);

		foreach ($filenames as $fn => $filename)
		{
			if (!is_file($filename))
			{
				if ($fn == 0)
				{
					file_put_contents("atlas2dupe.log", "Missing file: $filename - Skipping $egfname/$id\n", FILE_APPEND);
					continue 2;
				}
				else
				{
					file_put_contents("atlas2dupe.log", "Missing file: $filename - Truncating atlas $egfname/$id\n", FILE_APPEND);
					$filenames = array_slice($filenames, 0, $fn);
					$fileids = array_slice($fileids, 0, $fn);
					break;
				}
			}
		}

		$filename_args = implode(' ', $filenames);

		$tmpname = atlastmp_filename($egfname, $id);

		system("./eoatlas $tmpname $filename_args");

		$sha256 = substr(file_get_contents($tmpname . '.sha256'), 0, 16);

		$offs = array_map(
			function($a) { return explode(',', $a); },
			explode(';', file_get_contents($tmpname . '.offs2'))
		);

		$outputs[$sha256] = $tmpname;

		foreach ($fileids as $n => $id)
		{
			$table[$fileids[$n]] = array($sha256, $n, $offs[$n]);
		}
	}

	foreach ($outputs as $sha256 => $tmpname)
	{
		$imgname = gfx_filename($sha256);

		if (file_exists($imgname))
		{
			$imgname = gfxdupe_filename($sha256 . '-' . $egfname);
			file_put_contents("atlas2dupe.log", "Duplicate file: $imgname\n", FILE_APPEND);
		}

		if (isset($transfix[$sha256]))
		{
			system("convert $tmpname -background black -alpha remove $tmpname", $result);
		}

		system("pngcrush -q $tmpname $imgname", $result);

		if ($result != 0)
			exit("PNGCRUSH FAILED ON $sha256");
	}

	$tablename = ATLAS_PREFIX . '/' . str_replace('.egf', '.table', $egfname);

	write_table($tablename, $table);
}

function write_table($tablename, $table)
{
	$null_entry = array(
		"0000000000000000",
		0,
		array(0, 0, 0)
	);

	$maxid = 0;

	array_walk(
		$table,
		function($v, $k) use (&$maxid) { $maxid = max($maxid, $k); }
	);

	$fh = fopen($tablename, "wb");

	fwrite($fh, 'E2GT');
	fwrite($fh, pack('n', 1));
	fwrite($fh, pack('n', $maxid));

	for ($i = 1; $i <= $maxid; ++$i)
	{
		if (isset($table[$i]))
			$entry = $table[$i];
		else
			$entry = $null_entry;

		fwrite($fh, pack('H*', $entry[0]));
		fwrite($fh, pack('nnnn', $entry[1], $entry[2][0], $entry[2][1], $entry[2][2]));
	}

	fclose($fh);
}

function parse_instruction($instruction)
{
	$parts = explode(' ', $instruction, 2);

	if (count($parts) != 2)
		exit("Cannot parse: '$instruction'");

	return array($parts[0], expand_range($parts[1]));
}

function expand_range($rangestr)
{
	$range = array();
	$parts = explode(',', $rangestr);

	foreach ($parts as $part)
	{
		$part = trim($part);

		if (is_numeric($part))
		{
			$range[] = $part;
		}
		else
		{
			$parts2 = explode('..', $part, 2);

			if (count($parts2) == 2 && is_numeric($parts2[0]) && is_numeric($parts2[1]))
			{
				if ($parts2[0] <= $parts2[1])
				{
					for ($i = $parts2[0]; $i <= $parts2[1]; ++$i)
						$range[] = $i;
				}
				else
				{
					exit("Bad range: '$part' (in $rangestr)");
				}
			}
			else
			{
				exit("Cannot parse: '$part' (in $rangestr)");
			}
		}
	}

	return $range;
}

function make_custom_algo($instructions)
{
	$instructions = explode("\n", $instructions);

	$instructions = array_map(
		function($line) { return preg_replace( '#//.*#', '', $line); },
		$instructions
	);

	$instructions = array_map('trim', $instructions);

	$instructions = array_filter(
		$instructions,
		function ($line) { return strlen($line) > 0; }
	);

	$instructions = array_map('parse_instruction', $instructions);

	$packs = array_filter(
		$instructions,
		function ($instruction) { return $instruction[0] == 'PACK' || $instruction[0] == 'ANIM'; }
	);

	if (count($packs) > 0)
	{
		$minid = PHP_INT_MAX;
		$maxid = 0;

		foreach ($packs as $pack)
		{
			$minid = min($minid, array_reduce($pack[1], 'min', PHP_INT_MAX));
			$maxid = max($maxid, array_reduce($pack[1], 'max'));
		}

		$needs_auto = range($minid, $maxid);
		$needs_auto = array_combine($needs_auto, $needs_auto);

		foreach ($packs as $pack)
		{
			foreach ($pack[1] as $id)
			{
				if (isset($needs_auto[$id]))
					unset($needs_auto[$id]);
			}
		}

		foreach ($needs_auto as $id)
		{
			$packs[] = array(
				'AUTO',
				array($id)
			);
		}
	}

	// I used this to mark duplicate graphics but we have no way to alias
	//   duplicate graphics, so keep them around.
/*
	$packs = array_filter(
		$instructions,
		function ($instruction) { return $instruction[0] != 'CUT'; }
	);
*/

	$packs = array_values($packs);

	return function($i) use ($packs)
	{
		if ($i >= count($packs))
			return array();

		return array_map(function($i) { return $i - 100; }, $packs[$i][1]);
	};
}

function make_custom_set($set)
{
	return function($i) use ($set)
	{
		if ($i >= count($set))
			return array();

		return array($set[$i] - 100);
	};
}

system("rm ./atlas2/gfx/*.png");
system("rm ./atlas2/*.table");
system("rm ./atlas2dupe.log");

$atlases = array(
	// Interface sprites 1
	"gfx001.egf" => make_custom_set(array(
		101, 102,
		// 103-110 skipped
		111, 112, 113, 114, 115,
		118, 119, 120, 121, 122, 123, 124, 125,
		130, 131, 132, 133, 134, 135, 136, // backgrounds
		140,
		141, 142, 143, 144, // animus
		161, 162, 163, 164, 165, 166, 167, 168, // revenge of the animus
		200, 201, 202, // softpad
	)),

	// Interface sprites 2
	"gfx002.egf" => make_custom_set(array(
		101,
		120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
		130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
		140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
		150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167, 168, 169
	)),

	// Map sprites (ground)
	"gfx003.egf" => make_custom_algo(<<<endINSTRUCTIONS
		PACK 100
		PACK 101..102
		PACK 103..109
		PACK 110..113
		PACK 115,117,133..134
		PACK 119..120,135..136
		PACK 121,123,258
		PACK 124..132,239..244
		PACK 138..139
		PACK 142..161
		PACK 166..177,179..180
		PACK 178,181
		PACK 140,162..165,182..194
		PACK 195..202
		PACK 203..204
		PACK 205..206
		PACK 208..214
		PACK 217..218
		PACK 220..227,247
		PACK 228..229
		PACK 230..238
		PACK 245..246,248
		PACK 249..250
		PACK 251..257
		PACK 262..265
		PACK 266..267
		PACK 268..269
		PACK 270,308
		PACK 271..293
		PACK 294..297
		PACK 299..307
		PACK 309..311,327
		PACK 312,315,318,321,324
		PACK 313,316,319,322,325
		PACK 314,317,320,323,326
		PACK 335..349
		PACK 350..352
		PACK 353..365
		PACK 366..379
		PACK 382..386
		PACK 387..388
		PACK 389..390
		PACK 391..392
		PACK 393..394
		ANIM 396
		ANIM 397
		PACK 398..406
		PACK 407..415
		PACK 416..424
		PACK 425..432
		PACK 435..443
		PACK 444..452
		PACK 453..461
		PACK 462..475
		PACK 476..481,518..519
		PACK 482..492,524..527
		PACK 493..517,520..523,528..530
		PACK 531..549,556
		PACK 550..554
		ANIM 555
		PACK 557..560
		ANIM 561
		ANIM 562
		ANIM 563
		ANIM 564
		ANIM 565
		PACK 568..569
		PACK 570..588
		PACK 589..600,615
		ANIM 601
		PACK 602..613,616
		ANIM 614
		PACK 617..619
		PACK 620..622
		PACK 625,630
		PACK 627,628
		PACK 633..639,650
		PACK 640..647
		ANIM 648
		ANIM 649
		PACK 651..653,657
		PACK 654..656
		PACK 659..664,669
		PACK 665..667
		PACK 671..676
		PACK 677..678
		PACK 679..680
		PACK 681..682
		PACK 683..684
		ANIM 685
		PACK 686..702,704..707
		PACK 708..709
		PACK 710..713
		ANIM 716
		PACK 717..720
		PACK 722..726
		PACK 727..737,739..743,745..748
		PACK 738,744,749..750
		ANIM 751
		ANIM 752
		PACK 753..754,755..756,757..758
		ANIM 760
		PACK 770..773
		PACK 775..781
		PACK 784,799
		PACK 785..788
		PACK 789,791..798
		PACK 804..806
		PACK 807..809
		PACK 810..824
endINSTRUCTIONS
	),

	// Map sprites (objects)
	"gfx004.egf" => make_custom_algo(<<<endINSTRUCTIONS
		PACK 101..103,286..290
		PACK 109..111
		PACK 113..114
		PACK 116..117
		PACK 118..119
		PACK 120..123,347..350
		PACK 124..127
		PACK 128..135
		PACK 136..137
		PACK 141..142
		PACK 143..145
		PACK 147..148
		PACK 158..166
		PACK 171..172
		PACK 173..174
		PACK 175..176
		PACK 177..178
		PACK 179..181
		PACK 184,187
		PACK 194..196
		CUT 190 // DUPE OF 189
		PACK 203..206
		PACK 210..212,216..219
		PACK 213..215,220
		PACK 221..229
		PACK 230..237
		PACK 238..253
		PACK 254..255
		PACK 256..257
		PACK 258..259
		PACK 260..264
		PACK 265..271
		PACK 272..273
		PACK 274..277
		PACK 279..281
		PACK 282..285
		PACK 292..294
		PACK 295..296
		PACK 298..305
		PACK 306..307
		PACK 309..310
		PACK 311..316
		PACK 317..319
		PACK 322..323
		PACK 324..325
		PACK 327..328
		PACK 329,332
		PACK 330..331
		PACK 334..335
		PACK 337..338
		PACK 339..346
		PACK 354..357
		PACK 359..364
		PACK 365..374
		PACK 376..379
		PACK 383..386
		CUT 389 // DUPE OF 388
		PACK 388,390
		PACK 393..397
		PACK 398..401
		PACK 402..403
		PACK 408..414
		PACK 416..425
		PACK 426..429
		PACK 432..435
		PACK 437..438
		PACK 443..446
		PACK 452..456
		PACK 457..459
		PACK 460..461
		PACK 462..464
		PACK 465..467
		PACK 468..469,470..471,472..473
		PACK 474..475
		PACK 477..478
		PACK 481..484
		PACK 485..487
		PACK 489..490
		PACK 491..493
		PACK 494..496
		PACK 497..499
		PACK 502..504
		PACK 505..506
		PACK 508..510
		PACK 511..512
		PACK 513..518
		PACK 519..522
		PACK 523..527
		PACK 531..534
		PACK 535..538
		PACK 539..540
		PACK 541..542
		CUT 546 // DUPE OF 545
		CUT 547 // DUPE OF 545
		PACK 549..550
		PACK 553..554
		PACK 556..559
		PACK 560..562
		PACK 563..567
		PACK 568..570
		PACK 571..576
		PACK 578..580
		PACK 582..584
		PACK 591..595
		PACK 596..599
		PACK 600..605
		PACK 606..607
		PACK 608..610
		PACK 611..612
		PACK 614..615
		PACK 618..621
		PACK 626..629
		PACK 635..638
endINSTRUCTIONS
	),

	// Map sprites (overlay)
	"gfx005.egf" => make_custom_algo(<<<endINSTRUCTIONS
		PACK 104..105
		PACK 106..107
		PACK 111..112
		PACK 113..114
		CUT 121 // DUPE OF 118
		PACK 123..128
		PACK 134..135
endINSTRUCTIONS
	),

	// Map sprites (walls)
	"gfx006.egf" => make_custom_algo(<<<endINSTRUCTIONS
		PACK 101..102
		PACK 103..113
		PACK 114..115
		PACK 116..117
		PACK 118..119
		PACK 120..125
		PACK 126..129
		PACK 130..140
		PACK 141..149
		PACK 150..158
		PACK 159..167
		PACK 168..183
		PACK 184..191
		PACK 192..200,216
		PACK 201..215,217
		PACK 218..223
		PACK 224..231
		PACK 232..233
		PACK 234..238
		PACK 239..246
		PACK 247..250
		PACK 251..254
		PACK 255..258
		PACK 259..260
		PACK 261..264
		PACK 265..266
		PACK 267..271,280..284
		PACK 272..279
		PACK 285..288
		PACK 289..302
		PACK 303
		PACK 304..305
		PACK 306..308
		PACK 309..320
		PACK 321..328
		PACK 329..342
		PACK 343..346
		PACK 347..354
		PACK 355..356
		PACK 357..361
		PACK 362..363
		PACK 364..369
		PACK 370..375
		PACK 376..383
		PACK 384..392
		CUT 393 // DUPE OF 392
		PACK 394..395
		PACK 396..413,428..431
		PACK 414..427
		PACK 432..433
		PACK 434..435
		PACK 436..437
		PACK 438..439
		PACK 440..441
		PACK 442..443
		PACK 444..445
		PACK 446..447
		PACK 448..449
		PACK 450..457
		PACK 458..461
		PACK 462..473
		PACK 474..477
		PACK 478..489
		PACK 490..502
		PACK 503..516
		PACK 517..518
		PACK 519..530
		PACK 531..532 // (belongs elsewhere)
		PACK 533..543
		PACK 544..547 // (belongs elsewhere)
		PACK 548..552
		PACK 553..561
		PACK 562..591 // (belongs elsewhere - aeven building kit)
		PACK 592..597
		PACK 598..603
		PACK 604..633,638..639
		ANIM 634
		ANIM 635
		ANIM 636
		ANIM 637
		PACK 640..669,672..673
		ANIM 670
		ANIM 671
		PACK 674..677 // (belongs elsewhere - snow building kit)
		PACK 678..685
		PACK 686..689
		PACK 690..693
		PACK 694..695
		ANIM 696
		ANIM 697
		PACK 698..706
		ANIM 707
		PACK 708..728
		PACK 729..738
		PACK 739..752
		ANIM 753
		PACK 754..770
		PACK 771..776
		PACK 777..779
		PACK 780..785
		ANIM 786
		PACK 787..816
		PACK 817..870
		PACK 871..891
		PACK 892..904
		PACK 905..917
		PACK 918..934
		PACK 935..958
		PACK 959..966
		PACK 967..974
		ANIM 975
		PACK 976..979
		ANIM 980
		PACK 981..982
		ANIM 983
		ANIM 984
		PACK 985..986
		PACK 987..988
		PACK 989..1003
		PACK 989..1003,1005..1015
		ANIM 1004
		ANIM 1005
		CUT 1016 // DUPE OF 1015
		PACK 1017..1022
		PACK 1023..1024
		PACK 1025..1031
		CUT 1032..1037 // UNUSED
		PACK 1038..1047
		PACK 1048..1053
		PACK 1054..1068
		PACK 1069..1078
		PACK 1079..1082
		PACK 1083..1086
		PACK 1087..1090
		PACK 1091..1094
		PACK 1095..1102
		PACK 1103..1114
		ANIM 1115
		ANIM 1116
		PACK 1117..1124
		PACK 1125..1132
		PACK 1133..1141
		ANIM 1142
		ANIM 1143
		PACK 1144..1150
		PACK 1151..1162
endINSTRUCTIONS
	),

	// Map sprites (roof)
	"gfx007.egf" => make_custom_algo(<<<endINSTRUCTIONS
		PACK 101..102,104..105
		PACK 110..116
		PACK 117..125,128..129
		PACK 131..133
		PACK 134..137
		PACK 138..143
		PACK 144..145
endINSTRUCTIONS
	),

	// Character sprites
	"gfx008.egf" => function($i)
	{
		$base = $i + 1;

		if (100 + $base > 108)
			return array();

		return array($base);
	},

	// Male hairstyles
	"gfx009.egf" => function($i)
	{
		$base = 4 * $i + 1;

		if (100 + $base > 900)
			return array();

		return array($base + 1, $base + 3);
	},

	// Female hairstyles
	"gfx010.egf" => function($i)
	{
		$base = 4 * $i + 1;

		if (100 + $base > 900)
			return array();

		return array($base + 1, $base + 3);
	},

	// Male boots
	"gfx011.egf" => function($i)
	{
		$base = 40 * $i + 1;

		if (100 + $base > 2156)
			return array();

		return range($base, $base + 15);
	},

	// Female boots
	"gfx012.egf" => function($i)
	{
		$base = 40 * $i + 1;

		if (100 + $base > 2156)
			return array();

		return range($base, $base + 15);
	},

	// Male armours
	"gfx013.egf" => function($i)
	{
		$base = 50 * $i + 1;

		if (100 + $base > 2572)
			return array();

		return range($base, $base + 21);
	},

	// Female armours
	"gfx014.egf" => function($i)
	{
		$base = 50 * $i + 1;

		if (100 + $base > 2622)
			return array();

		return range($base, $base + 21);
	},

	// Male hats
	"gfx015.egf" => function($i)
	{
		$base = 10 * $i + 1;

		if (100 + $base > 593)
			return array();

		return range($base, $base + 2);
	},

	// Female hats
	"gfx016.egf" => function($i)
	{
		$base = 10 * $i + 1;

		if (100 + $base > 593)
			return array();

		return range($base, $base + 2);
	},

	// Male weapons
	"gfx017.egf" => function($i)
	{
		$base = 100 * $i + 1;

		if (100 + $base > 7417)
			return array();

		return range($base, $base + 16);
	},

	// Female weapons
	"gfx018.egf" => function($i)
	{
		$base = 100 * $i + 1;

		if (100 + $base > 7417)
			return array();

		return range($base, $base + 16);
	},

	// Male shields
	"gfx019.egf" => function($i)
	{
		$base = 50 * $i + 1;

		if (100 + $base > 1066)
			return array();

		return range($base, $base + 15);
	},

	// Female shields
	"gfx020.egf" => function($i)
	{
		$base = 50 * $i + 1;

		if (100 + $base > 1066)
			return array();

		return range($base, $base + 15);
	},

	// NPC Sprites
	"gfx021.egf" => function($i)
	{
		$base = 40 * $i + 1;

		if (100 + $base > 6878)
			return array();

		return range($base, $base + 15);
	},

	// Map sprites (shadows)
	"gfx022.egf" => function($i)
	{
		$base = $i + 1;

		if (100 + $base > 169)
			return array();

		return array($base);
	},

	// Item sprites
	"gfx023.egf" => function($i)
	{
		$base = 2 * $i + 1;

		if (100 + $base > 1048)
			return array();

		return array($base, $base + 1);
	},

	// Spell animations
	"gfx024.egf" => function($i)
	{
		$base = $i + 1;

		if (100 + $base > 201)
			return array();

		return array($base);
	},

	// Spell icons
	"gfx025.egf" => function($i)
	{
		$base = $i + 1;

		if (100 + $base > 114)
			return array();

		return array($base);
	}
);

echo "Packing and optimizing graphics";

foreach ($atlases as $egf => $algo)
{
	echo ".";
	generate_atlas_sane($egf, $algo);
}

echo "\n";
