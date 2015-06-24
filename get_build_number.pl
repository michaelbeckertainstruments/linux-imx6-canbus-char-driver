#!/usr/bin/perl

$version_file = "version.h";


open (VER_FILE, "< $version_file") or die "Can't open $version_file: $!\n";

while (<VER_FILE>){
    
    if (@str_ver = $_ =~ /^MODULE_VERSION[\t ]*\([\t ]*\"(.*)\"[\t ]*\)/){
        @version_numbers = $str_ver[0] =~ /([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)/;
        print "$version_numbers[0].$version_numbers[1].$version_numbers[2].$version_numbers[3]\n";
    }
}


close VER_FILE;


#
#   Tell make or our shell script(s) that this worked.
#
exit 0;

