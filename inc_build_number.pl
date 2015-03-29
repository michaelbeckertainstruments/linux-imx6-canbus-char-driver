#!/usr/bin/perl

$version_file = "version.h";
$tmp_file = "version.tmp";


open (VER_FILE, "< $version_file") or die "Can't open $version_file: $!\n";
open (TMP_FILE, "> $tmp_file") or die "Can't open $tmp_file $!\n";

while (<VER_FILE>){
    
    if (@str_ver = $_ =~ /^MODULE_VERSION[\t ]*\([\t ]*\"(.*)\"[\t ]*\)/){
        @version_numbers = $str_ver[0] =~ /([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)/;
        $version_numbers[3]++;
        print TMP_FILE "MODULE_VERSION(\"";
        print TMP_FILE "$version_numbers[0].$version_numbers[1].$version_numbers[2].$version_numbers[3]";
        print TMP_FILE "\");\n";
    }
    else{
        print TMP_FILE $_;
    }
}


close VER_FILE;
close TMP_FILE;

rename ($tmp_file, $version_file) or die "Can't rename $tmp_file to $version_file: $!\n";


#
#   Tell make or our shell script(s) that this worked.
#
exit 0;

