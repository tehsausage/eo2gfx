#!/usr/bin/env php
<?php

define('EGF_PREFIX', './gfx/');
define('OUTPUT_PREFIX', './dump/');

$this_time = filemtime(__FILE__);
$gfxdump_time = filemtime("./gfxdump");

$egfs = array(
	'gfx001.egf',
	'gfx002.egf',
	'gfx003.egf',
	'gfx004.egf',
	'gfx005.egf',
	'gfx006.egf',
	'gfx007.egf',
	'gfx008.egf',
	'gfx009.egf',
	'gfx010.egf',
	'gfx011.egf',
	'gfx012.egf',
	'gfx013.egf',
	'gfx014.egf',
	'gfx015.egf',
	'gfx016.egf',
	'gfx017.egf',
	'gfx018.egf',
	'gfx019.egf',
	'gfx020.egf',
	'gfx021.egf',
	'gfx022.egf',
	'gfx023.egf',
	'gfx024.egf',
	'gfx025.egf'
);

function dump_egf($egf)
{
	global $this_time;
	global $gfxdump_time;

	$egf_filename = EGF_PREFIX . $egf;
	$output_dirname = OUTPUT_PREFIX . $egf;
	$egf_time = max($this_time, $gfxdump_time, filemtime($egf_filename));

	if (!file_exists($output_dirname) || $egf_time > filemtime($output_dirname))
	{
		system("./gfxdump -t $egf_filename");
		touch($output_dirname);
	}
}

echo "Dumping EGF files";

foreach ($egfs as $egf)
{
	echo ".";
	dump_egf($egf);
}

echo "\n";
