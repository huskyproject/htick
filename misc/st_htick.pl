#!/usr/bin/perl
#
# h-tick log analyzer
#   by Dmitry Pankov (2:5022/81)
#
# Based on
#   hptlogstat.pl by Yuriy Daybov (2:5029/42)
#   dmstat.pl by Eugene Barbashin (2:5030/920)
#
# Usage:
#   Set $logname to point to your H-Tick logfile. Then call the script:
#   st_htick.pl [days]
#
# Examples:
#   "st_htick.pl 7" - create 7-day statistics for all the areas
#   "st_htick.pl 1" - statistics for last day

use Time::Local;

my %areaq;
my %areas;
my %linkq;
my %links;

my $cur_size;
my $totals = 0;
my $totalq = 0;

#$logname = "c:\\fido\\logs\\htick.log";
$logname = "/fido/log/htick.log";

# this hash is used, when converting verbose months to numeral (Jan = 0)
@months{qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec)} = (0..11);
# working with command line arguments
if ($#ARGV > 1) {
    print("Wrong command line arguments number\n");
    exit;
}
foreach (@ARGV) {
    if(/^\d{1,4}$/ && !$period) { $period = $_ }
}
$date = time() - (24 * 60 * 60 * ($period - 1)) if $period;

open(LOG, "<$logname") || die "can't open $logname: $!";
while (<LOG>) {
    if (/-{10}\s+\w+\s(.*),/) {
        $last = $1;
        if (!$from) {
            $found = date_to_period($1);
            $date ||= $found;
            $from = $1 if ($found >= $date);
        }
        next;
    }
## log format changed ##################
#    if ($from) {
#        if (/Size: (\d+)/) {
#            $totals += $1;
#            $totalq++;
#            $cur_size = $1;
#        };
#        if (/Area: (\S+)/) {
#            $areaq{"\U$1"}++;
#            $areas{"\U$1"} += $cur_size;
#        };
#        if (/From: (\S+)/) {
#            $linkq{$1}++;
#            $links{$1} += $cur_size;
#        }
#    }
# ######################################
    if ($from) {
        if (/size: (\d+) area: (\S+) from: (\S+)/) {
            $totals += $1;
            $totalq++;
            $cur_size = $1;
            $areaq{"\U$2"}++;
            $areas{"\U$2"} += $cur_size;
            $linkq{$3}++;
            $links{$3} += $cur_size;
        }
    }

}
close LOG || die "Can't close $logname: $!";;

# error checking
unless(%areas) {
    print "No files in areas or wrong period of days!\n";
    exit;
}
$period ||= int ((date_to_period($last) - $date) /24 /60 /60 + 1);

# results
print "\n\n> Fileecho traffic from $from to $last\n\n";
#print "ษออออออออออออออออออออออออออออออออออออออออออออออัออออออออออั".
#       "ออออออออออออออป";
#printf "\nบ%16c%-29s ณ %8s ณ %12s บ\n", 32, "Fileecho area", "Files", "Bytes";
#print "วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤลฤฤฤฤฤฤฤฤฤฤล".
#       "ฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ\n";

print   "+==============================================+==========+",
        "==============+";
printf "\n|%16c%-29s | %8s | %12s |\n", 32, "Fileecho area", "Files", "Bytes";
print   "+==============================================+==========+",
        "==============+";

foreach (sort (keys %areaq)) {
#  printf "บ %-44s ณ %8s ณ %12s บ\n", $_, $areaq{$_}, $areas{$_};
  printf "| %-44s | %8s | %12s |\n", $_, $areaq{$_}, $areas{$_};
}

#print "ศออออออออออออออออออออออออออออออออออออออออออออออฯออออออออออฯ".
#       "ออออออออออออออผ\n";
#print "ษออออออออออออออออออออออออออออออออออออออออออออออัออออออออออั".
#       "ออออออออออออออป";
#printf "\nบ%14c%-31s ณ %8s ณ %12s บ\n", 32,
#        "Fileecho uplinks", "Files", "Bytes";
#print "วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤลฤฤฤฤฤฤฤฤฤฤล".
#       "ฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ\n";

print   "+==============================================+==========+",
        "==============+";
printf "\n|%14c%-31s | %8s | %12s |\n", 32, "Fileecho area", "Files", "Bytes";
print   "+==============================================+==========+",
        "==============+\n";


foreach (sort (keys %linkq)) {
#  printf "บ %-44s ณ %8s ณ %12s บ\n", $_, $linkq{$_}, $links{$_};
  printf "| %-44s | %8s | %12s |\n", $_, $linkq{$_}, $links{$_};
}
#print "ศออออออออออออออออออออออออออออออออออออออออออออออฯออออออออออฯ".
#"ออออออออออออออผ\n";
print   "+==============================================+==========+",
        "==============+\n";

{
  my $totala = keys(%areaq);
  my $totall = keys(%linkq);

  print "\n  Total $totals byte(s) in $totalq file(s)";
  print " from $totala fileecho(s) from $totall link(s)\n";
}

# converting verbose date to epoch seconds
sub date_to_period {
    $_[0] =~ /(\d\d)\s(\w\w\w)\s(\d\d)/i;
    ($day, $month, $year) = ($1, $2, $3);
    timelocal("59", "59", "23", $day, $months{$month}, $year);
}
