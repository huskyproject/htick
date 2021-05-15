#!/usr/bin/perl
#
# h-tick log analyzer
#   by Dmitry Pankov (2:5022/81)
# 
# Based on
#   hptlogstat.pl by Yuriy Daybov (2:5029/42)
#   dmstat.pl by Eugene Barbashin (2:5030/920)
#
# Updated for htick 1.9.0-cur
#   by Alexander Kruglikov (2:5053/58)
#
# Usage:
#   Set $logname to point to your H-Tick logfile. Then call the script:
#   st_htick.pl [days]
#
# Examples:
#   "st_htick.pl 7" - create 7-day statistics for all the areas
#   "st_htick.pl 1" - statistics for last day

use strict;
use warnings;
use Time::Local;

my %areaq;
my %areas;
my %linkq;
my %links;

my $cur_size;
my $totals = 0;
my $totalq = 0;

#$logname = "c:\\fido\\logs\\htick.log";
my $logname = "/home/fido/log/htick.log";

# this hash is used, when converting verbose months to numeral (Jan = 0)
my %months;
@months{qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec)} = (0..11);

# working with command line arguments
if($#ARGV > 1)
{
    print("Wrong command line arguments number\n");
    exit;
}
my $period;
foreach (@ARGV)
{
    if(/^\d{1,4}$/ && !$period)
    {
        $period = $_ 
    }
}

my $date = time() - (24 * 60 * 60 * ($period - 1)) if $period;

my ($last, $from, );
open(LOG, "<", "$logname") or die "can't open $logname: $!";
while(<LOG>)
{
    if(/-{10}\s+\w+\s(.*),/)
    {
        $last = $1;
        if(!$from)
        {
            my $found = date_to_unixtime($1);
            $date ||= $found;
            $from = $1 if ($found >= $date);
        }
        next;
    }

    if($from)
    {
        if(/size: (\d+|not specified), area: (\S+), from: (\S+),/)
        {
            $totals += $1 eq "not specified" ? 0 : $1;
            $totalq++;
            $cur_size = $1 eq "not specified" ? 0 : $1;
            $areaq{"\U$2"}++;
            $areas{"\U$2"} += $cur_size;
            $linkq{$3}++;
            $links{$3} += $cur_size;
        }
    }
}
close LOG || die "Can't close $logname: $!";;

# error checking
unless(%areas)
{
    print "No files in areas or wrong period of days!\n";
    exit;
}

$period ||= int ((date_to_unixtime($last) - $date) /24 /60 /60 + 1);

# results
print "\n\n> Fileecho traffic from $from to $last\n\n";

print   "+==============================================+==========+",
        "===============+";
printf "\n|%16c%-29s | %8s | %13s |\n", 32, "Fileecho area", "Files", "Bytes";
print   "+==============================================+==========+",
        "===============+\n";

foreach (sort (keys %areaq))
{
  printf "| %-44s | %8s | %13s |\n", $_, $areaq{$_}, $areas{$_};
}

print   "+==============================================+==========+",
        "===============+";
printf "\n|%14c%-31s | %8s | %13s |\n", 32, "Fileecho uplinks", "Files",
       "Bytes";
print   "+==============================================+==========+",
        "===============+\n";


foreach (sort (keys %linkq))
{
  printf "| %-44s | %8s | %13s |\n", $_, $linkq{$_}, $links{$_};
}
print   "+==============================================+==========+",
        "===============+\n";

{
  my $totala = keys(%areaq);
  my $totall = keys(%linkq);

  print "\n  Total $totals byte(s) in $totalq file(s)";
  print " from $totala fileecho(s) from $totall link(s)\n";
}

# convert date in the format "04 May 2021" to unix time in seconds
sub date_to_unixtime
{
    $_[0] =~ /(\d{2})\s(\w{3})\s(\d{4})/;
    my ($day, $month, $year) = ($1, $2, $3);
    timelocal("59", "59", "23", $day, $months{$month}, $year);
}
