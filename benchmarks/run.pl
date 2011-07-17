#!/usr/bin/env perl
use strict;
use warnings;

if (!defined $ARGV[0]) {
  print "Usage: run.pl <path_to_sim>\n";
  exit(1);
}
my $axe = $ARGV[0];

sub timeCommand
{
  my $cmd = shift;
  my ($start_user, $start_system, $start_child_user, $start_child_system) = times();
  system("$cmd > /dev/null 2>&1");
  my ($end_user, $end_system, $end_child_user, $end_child_system) = times();
  return ($end_child_user + $end_child_system) - ($start_child_user + $start_child_system);
}

sub printFrequency
{
  my $hz = shift;
  if ($hz >= 1000000) {
    my $mhz = $hz / 1000000;
    printf("%.2f MHz", $mhz);
  } elsif ($hz >= 1000) {
    my $khz = $hz / 1000;
    printf("%.2f kHz", $khz);
  } else {
    printf("%.2f Hz", $hz);
  }
}

sub getXSimCycles
{
  my $args = shift;
  open(STATS, "xsim $args --stats|");
  my $cycles;
  while (<STATS>) {
    if (/^SimulationCycles : (\d+)/) {
      $cycles = $1;
    }
  }
  if (!defined($cycles)) {
    die("Unable to determine number of cycles,");
  }
  close(STATS);
  return $cycles;
}

sub doBench
{
  my $benchmarkName = shift;
  my $compileCmd = shift;
  my $xsimRunArgs = shift;
  my $axeRunArgs = shift;
  my $xsimRuns = shift;
  my $axeRuns = shift;

  my $calibration_runs_1 = int($xsimRuns / 4);
  my $calibration_runs_2 = int($xsimRuns / 2);
  print("Benchmark: $benchmarkName\n");
  system("$compileCmd -DRUNS=$calibration_runs_1");
  my $cycles1 = getXSimCycles($xsimRunArgs);
  system("$compileCmd -DRUNS=$calibration_runs_2");
  my $cycles2 = getXSimCycles($xsimRunArgs);
  my $runCycles = ($cycles2 - $cycles1) / ($calibration_runs_2 - $calibration_runs_1);
  my $baseCycles = $cycles1 - $runCycles * $calibration_runs_1;

  system("$compileCmd -DRUNS=$xsimRuns");
  my $xsimTime = timeCommand("xsim $xsimRunArgs");
  my $cycles = $baseCycles + $xsimRuns * $runCycles;
  my $hz = $cycles / $xsimTime;
  print("xsim: ");
  printFrequency($hz);
  print("\n");

  system("$compileCmd -DRUNS=$axeRuns");
  my $axeTime = timeCommand("$axe $axeRunArgs");
  $cycles = $baseCycles + $axeRuns * $runCycles;
  $hz = $cycles / $axeTime;
  print("axe: ");
  printFrequency($hz);
  print("\n");
}

sub main
{
  doBench(
    "dhrystone",
    "xcc -w dhry_1.c dhry_2.c -O2 -target=XC-5 -o dhry.xe",
    "dhry.xe",
    "dhry.xe",
    1000,
    2000000
  );
  doBench(
    "ports",
    "xcc ports.xc -O2 -target=XC-5 -o ports.xe",
    "ports.xe --plugin LoopbackPort.so '-port stdcore[0] XS1_PORT_1A 1 0 -port stdcore[0] XS1_PORT_1E 1 0 -port stdcore[0] XS1_PORT_1B 1 0 -port stdcore[0] XS1_PORT_1G 1 0 -port stdcore[0] XS1_PORT_1C 1 0 -port stdcore[0] XS1_PORT_1F 1 0 -port stdcore[0] XS1_PORT_1D 1 0 -port stdcore[0] XS1_PORT_1H 1 0 -port stdcore[0] XS1_PORT_1I 1 0 -port stdcore[0] XS1_PORT_1M 1 0 -port stdcore[0] XS1_PORT_1J 1 0 -port stdcore[0] XS1_PORT_1O 1 0 -port stdcore[0] XS1_PORT_1K 1 0 -port stdcore[0] XS1_PORT_1N 1 0 -port stdcore[0] XS1_PORT_1L 1 0 -port stdcore[0] XS1_PORT_1P 1 0'",
    "ports.xe --loopback 0x10200 0x10600 --loopback 0x10000 0x10500 --loopback 0x10100 0x10400 --loopback 0x10300 0x10700 --loopback 0x10a00 0x10c00 --loopback 0x10800 0x10e00 --loopback 0x10900 0x10d00 --loopback 0x10b00 0x10f00",
    100,
    1000000
  );
}

main();
