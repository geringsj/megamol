#!/usr/bin/perl
#
# configure.win.pl
# MegaMol Plugin
#
# Copyright (C) 2008-2010 by VISUS (Universitaet Stuttgart).
# Alle Rechte vorbehalten.
#
use Cwd qw{abs_path};
use strict;
use warnings 'all';
my $incpath = abs_path($0);
$incpath =~ s/\/[^\/]+$//;
push @INC, "$incpath/configperl";
require configperl;

my ($a, $b, $c);
my @pps = ();
my @fps = ();
my @cfps = ();
my @sps = ();

$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("outbin");
    $a->description("Path to the global \"bin\" output directory");
    $a->placeholder("%outbin%");
    $a->autoDetect(0);
    $a->value("../bin");
    push @pps, $a;

$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);

    $a->id("mmcore");
    $a->description("Path to the MegaMol core directory");
    $a->placeholder("%mmcorePath%");
    $a->markerFile("api/MegaMolCore.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("vislib");
    $a->description("Path to the vislib directory");
    $a->placeholder("%vislib%");
    $a->markerFile("vislib.sln\$");
    $a->relativeLocation("./");
    $a->autoDetect(1);
    push @pps, $a;
$b = FlagParameter->new();
    $b->id("withNETCDF");
    $b->description("Enable the use of the NetCDF library");
    $b->placeholder("%withNETCDF%");
    $b->value(0);
    push @fps, $b;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("netcdf");
    $a->description("Path to the NetCDF directory");
    $a->placeholder("%netcdfPath%");
#    $a->markerFile("netcdf.h\$");
#    $a->relativeLocation("../");
    $a->autoDetect(0);
    $a->dependencyFlagID("withNETCDF");
    $a->dependencyDisabledValue("");
    push @pps, $a;
$b = FlagParameter->new();
    $b->id("withOPENCL");
    $b->description("Enable the use of OpenCL");
    $b->placeholder("%withOPENCL%");
    $b->value(0);
    push @fps, $b;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("opencl");
    $a->description("Path to the NVIDIA GPU Computing SDK directory");
    $a->placeholder("%openclPath%");
    $a->autoDetect(0);
    $a->dependencyFlagID("withOPENCL");
    $a->dependencyDisabledValue("");
    push @pps, $a;
$b = FlagParameter->new();
    $b->id("withCUDA");
    $b->description("Enable the use of CUDA");
    $b->placeholder("%withCUDA%");
    $b->value(0);
    push @fps, $b;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("cuda");
    $a->description("Path to the NVIDIA GPU Computing SDK directory");
    $a->placeholder("%cudaPath%");
    $a->autoDetect(0);
    $a->dependencyFlagID("withCUDA");
    $a->dependencyDisabledValue("");
    push @pps, $a;
$b = FlagParameter->new();
    $b->id("withHAPTICS");
    $b->description("Enable the use of SensAble OpenHaptics");
    $b->placeholder("%withHAPTICS%");
    $b->value(0);
    push @fps, $b;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->id("haptics");
    $a->description("Path to the OpenHaptics (3DTouch) directory");
    $a->placeholder("%hapticsPath%");
    $a->autoDetect(0);
    $a->dependencyFlagID("withHAPTICS");
    $a->dependencyDisabledValue("");
    push @pps, $a;

$c = ConfigFilePair->new();
    $c->inFile("ExtLibs.props.input");
    $c->outFile("ExtLibs.props");
    push @cfps, $c;

VISUS::configperl::Configure("MegaMol(TM) Plugin Configuration for Windows", ".megamol.plg.win.cache", \@pps, \@fps, \@cfps, \@sps);

