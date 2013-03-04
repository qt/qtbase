#!/usr/bin/perl -w

use Cwd;
use Cwd 'abs_path';
use File::Basename;
use File::Temp 'tempdir';
use File::Path 'remove_tree';
use Getopt::Long;
use Pod::Usage;

### default options
my @stack = cwd;
my $device_serial=""; # "-s device_serial";
my $packageName="org.qtproject.qt5.android.tests";
my $intentName="$packageName/org.qtproject.qt5.android.QtActivity";
my $jobs = 4;
my $testsubset = "";
my $man = 0;
my $help = 0;
my $make_clean = 0;
my $deploy_qt = 0;
my $time_out=400;
my $android_sdk_dir = "$ENV{'HOME'}/NecessitasQtSDK/android-sdk";
my $ant_tool = `which ant`;
chomp $ant_tool;
my $strip_tool="";
my $readelf_tool="";
GetOptions('h|help' => \$help
            , man => \$man
            , 's|serial=s' => \$device_serial
            , 't|test=s' => \$testsubset
            , 'c|clean' => \$make_clean
            , 'd|deploy' => \$deploy_qt
            , 'j|jobs=i' => \$jobs
            , 'sdk=s' => \$android_sdk_dir
            , 'ant=s' => \$ant_tool
            , 'strip=s' => \$strip_tool
            , 'readelf=s' => \$readelf_tool
            ) or pod2usage(2);
pod2usage(1) if $help;
pod2usage(-verbose => 2) if $man;

my $adb_tool="$android_sdk_dir/platform-tools/adb";
system("$adb_tool devices") == 0 or die "No device found, please plug/start at least one device/emulator\n"; # make sure we have at least on device attached

$device_serial = "-s $device_serial" if ($device_serial);
$testsubset="/$testsubset" if ($testsubset);

$strip_tool="$ENV{'HOME'}/NecessitasQtSDK/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-strip" unless($strip_tool);
$readelf_tool="$ENV{'HOME'}/NecessitasQtSDK/android-ndk/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-readelf"  unless($readelf_tool);
$readelf_tool="$readelf_tool -d -w ";

sub dir
{
#    print "@stack\n";
}

sub pushd ($)
{
    unless ( chdir $_[0] )
    {
        warn "Error: $!\n";
        return;
    }
    unshift @stack, cwd;
    dir;
}

sub popd ()
{
    @stack > 1 and shift @stack;
    chdir $stack[0];
    dir;
}


sub waitForProcess
{
    my $process=shift;
    my $action=shift;
    my $timeout=shift;
    my $sleepPeriod=shift;
    $sleepPeriod=1 if !defined($sleepPeriod);
    print "Waiting for $process ".$timeout*$sleepPeriod." seconds to";
    print $action?" start...\n":" die...\n";
    while ($timeout--)
    {
        my $output = `$adb_tool $device_serial shell ps 2>&1`; # get current processes
        #FIXME check why $output is not matching m/.*S $process\n/ or m/.*S $process$/ (eol)
        my $res=($output =~ m/.*S $process/)?1:0; # check the procress
        if ($action == $res)
        {
            print "... succeed\n";
            return 1;
        }
        sleep($sleepPeriod);
        print "timeount in ".$timeout*$sleepPeriod." seconds\n"
    }
    print "... failed\n";
    return 0;
}

my $src_dir_qt=abs_path(dirname($0)."/..");
my $quadruplor_dir="$src_dir_qt/tests/auto/android";
my $qmake_path="$src_dir_qt/bin/qmake";
my $tests_dir="$src_dir_qt/tests$testsubset";
my $temp_dir=tempdir(CLEANUP => 1);
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
my $output_dir=$stack[0]."/".(1900+$year)."-$mon-$mday-$hour:$min";
mkdir($output_dir);
my $sdk_api=0;
my $output = `$adb_tool $device_serial shell getprop`; # get device properties
if ($output =~ m/.*\[ro.build.version.sdk\]: \[(\d+)\]/)
{
    $sdk_api=int($1);
    $sdk_api=5 if ($sdk_api>5 && $sdk_api<8);
    $sdk_api=9 if ($sdk_api>9);
}

sub reinstallQuadruplor
{
    pushd($quadruplor_dir);
    system("$android_sdk_dir/tools/android update project -p . -t android-4")==0 or die "Can't update project ...\n";
    system("$ant_tool uninstall clean debug install")==0 or die "Can't install Quadruplor\n";
    system("$adb_tool $device_serial shell am start -n $intentName"); # create application folders
    waitForProcess($packageName,1,10);
    waitForProcess($packageName,0,20);
    popd();
}
sub killProcess
{
    reinstallQuadruplor;
# #### it seems I'm too idiot to use perl regexp
#     my $process=shift;
#     my $output = `$adb_tool $device_serial shell ps 2>&1`; # get current processes
#     $output =~ s/\r//g; # replace all "\r" with ""
#     chomp($output);
#     print $output;
#     if ($output =~ m/^.*_\d+\s+(\d+).*S $process/) # check the procress
#     {
#         print("Killing $process PID:$1\n");
#         system("$adb_tool $device_serial shell kill $1");
#         waitForProcess($process,0,20);
#     }
#     else
#     {
#         print("Can't kill the process $process\n");
#     }
}


sub startTest
{
    my $libs = shift;
    my $mainLib = shift;
    my $openGL = ((shift)?"true":"false");
    system("$adb_tool $device_serial shell am start -n $intentName --ez needsOpenGl $openGL --es extra_libs \"$libs\" --es lib_name \"$mainLib\""); # start intent
    #wait to start
    return 0 unless(waitForProcess($packageName,1,10));
    #wait to stop
    unless(waitForProcess($packageName,0,$time_out,5))
    {
        killProcess($packageName);
        return 1;
    }
    my $output_file = shift;
    system("$adb_tool $device_serial pull /data/data/$packageName/app_files/output.xml $output_dir/$output_file");
    return 1;
}

sub needsOpenGl
{
    my $app=$readelf_tool.shift.' |grep -e "^.*(NEEDED).*Shared library: \[libQtOpenGL\.so\]$"';
    my $res=`$app`;
    chomp $res;
    return $res;
}

########### delpoy qt libs ###########
if ($deploy_qt)
{

    pushd($src_dir_qt);
    mkdir("$temp_dir/lib");
    my @libs=`find lib -name *.so`; # libs must be handled diferently
    foreach (@libs)
    {
        chomp;
        print ("cp -L $_ $temp_dir/lib\n");
        system("cp -L $_ $temp_dir/lib");
    }
    system("cp -a plugins $temp_dir");
    system("cp -a imports $temp_dir");
    pushd($temp_dir);
    system("find -name *.so | xargs $strip_tool --strip-unneeded");
    popd;
    system("$adb_tool $device_serial shell rm -r /data/local/qt"); # remove old qt libs
    system("$adb_tool $device_serial push $temp_dir /data/local/qt"); # copy newer qt libs
    popd;
}

########### build & install quadruplor ###########
reinstallQuadruplor;

########### build qt tests and benchmarks ###########
pushd($tests_dir);
system("make distclean") if ($make_clean);
system("$qmake_path CONFIG-=QTDIR_build -r") == 0 or die "Can't run qmake\n"; #exec qmake
system("make -j$jobs") == 0 or warn "Can't build all tests\n"; #exec make
my $testsFiles=`find . -name libtst_*.so`; # only tests
foreach (split("\n",$testsFiles))
{
    chomp; #remove white spaces
    pushd(abs_path(dirname($_))); # cd to application dir
    system("make INSTALL_ROOT=$temp_dir install"); # install the application to temp dir
    system("$adb_tool $device_serial shell rm -r /data/data/$packageName/app_files/*"); # remove old data
    system("$adb_tool $device_serial push $temp_dir /data/data/$packageName/app_files"); # copy
    my $application=basename(cwd);
    my $output_name=dirname($_);
    $output_name =~ s/\.//;   # remove first "." character
    $output_name =~ s/\///;   # remove first "/" character
    $output_name =~ s/\//_/g; # replace all "/" with "_"
    $output_name=$application unless($output_name);
    $time_out=5*60/5; # 5 minutes time out for a normal test
    if (-e "$temp_dir/libtst_bench_$application.so")
    {
        $time_out=5*60/5; # 10 minutes for a benchmark
        $application = "bench_$application";
    }

    if (-e "$temp_dir/libtst_$application.so")
    {
        if (needsOpenGl("$temp_dir/libtst_$application.so"))
        {
             startTest("/data/local/qt/plugins/platforms/android/libandroidGL-$sdk_api.so", "/data/data/$packageName/app_files/libtst_$application.so", 1
                        , "$output_name.xml") or warn "Can't run $application ...\n";
        }
        else
        {
             startTest("/data/local/qt/plugins/platforms/android/libandroid-$sdk_api.so", "/data/data/$packageName/app_files/libtst_$application.so", 0
                        , "$output_name.xml") or warn "Can't run $application stopping tests ...\n";
        }
    }
    else
    {   #ups this test application doesn't respect name convention
        warn "$application test application doesn't respect name convention please fix it !\n";
    }
    popd();
    remove_tree( $temp_dir, {keep_root => 1} );
}
popd();

__END__

=head1 NAME

Script to run all qt tests/benchmarks to an android device/emulator

=head1 SYNOPSIS

runtests.pl [options]

=head1 OPTIONS

=over 8

=item B<-s --serial = serial>

Device serial number. May be empty if only one device is attached.

=item B<-t --test = test_subset>

Tests subset (e.g. benchmarks, auto, auto/qbuffer, etc.).

=item B<-d --deploy>

Deploy current qt libs.

=item B<-c --clean>

Clean tests before building them.

=item B<-j --jobs = number>

Make jobs when building tests.

=item B<--sdk = sdk_path>

Android SDK path.

=item B<--ant = ant_tool_path>

Ant tool path.

=item B<--strip = strip_tool_path>

Android strip tool path, used to deploy qt libs.

=item B<--readelf = readelf_tool_path>

Android readelf tool path, used to check if a test application uses qt OpenGL.

=item B<-h  --help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=back

=head1 DESCRIPTION

B<This program> will run all qt tests/benchmarks to an android device/emulator.

=cut
