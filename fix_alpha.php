#!/usr/bin/env php
<?php

$noalpha = array(
	"03561109367b7769",
	"10def4cc9250f2e9",
	"266cb426c6a4eb3d",
	"2c42cf216f692e5e",
	"2dc3c3eed9e6185d",
	"379d6431abd0f8c7",
	"3cb6839eefe6a4c0",
	"44d6ba5861286dda",
	"4598779055e5aede",
	"50f777571f6c75fd",
	"5a9762a5f6e0b44c",
	"80d81397a61b5404",
	"8fdeec6d0190c10d",
	"993447253c557ff5",
	"a59599ea580bde3e",
	"a6d18ac36b1f593d",
	"be69005292c753b9",
	"bfefbc72d8b6b7a6",
	"c457c892cbab5267",

	"120b1da05647f9c2",
	"1b1509392f774a80",
	"26437c8886909984",
	"324a7e599b3db195",
	"3745621c8a062428",
	"37559ae24d8aff39",
	"398981d5e436d755",
	"3a9ea6bd302dfc58",
	"423d2451d412ca7c",
	"4ee5c4c2c351c305",
	"5d08208bae504ae6",
	"696a37eedbeeeee4",
	"6f2323f17d76b52b",
	"7ce823390291eb8c",
	"83e55d763940bc50",
	"94ac24abf0c548a5",
	"a95496851762f1ea",
	"ac0546a1a3722e6e",
	"afa9691b85818f99",
	"b970115f076033ad",
	"ba1d09d4008ab072",
	"be294294f803f201",
	"bf84183a2a29abc1",
	"c375a1eef9398c7a",
	"c3cd9ed818756cb9",
	"cab957f311940de1",
	"cae3431b31f3b244",
	"ceda191b56fb0271",
	"d009d644104e78d2",
	"d73068fb1318862b",
	"d94e98c874fff835",
	"e186a3a797a5f8c3",
	"f8db3cedf5c48d9b",
	"fbebd6c3f4468450",
);

foreach ($noalpha as $sha256)
{
	$gfxname = 'atlas/gfx/' . $sha256 . '.png';
	$tmpname = 'atlastmp/x.png';
	echo "Fixing $sha256...\n";

	system("convert $gfxname -background black -alpha remove $tmpname");
	system("pngcrush -q $tmpname $gfxname");
}

