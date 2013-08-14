#!/usr/bin/perl -Tw

# bindgraph -- a BIND statistics rrdtool frontend
# copyright (c) 2003 Marco Delaurenti <dela@linux.it>
# copyright (c) 2003 Marco d'Itri <md@linux.it>
# based on mailgraph (c) David Schweikert <dws@ee.ethz.ch>
# Released under the terms of the GNU General Public License.

use RRDs;
use POSIX qw(uname);
use File::Basename;
use strict;

my $VERSION = '0.2';

# hostname. will be printed in the HTML page
my $hostname = (POSIX::uname())[1];
# script name, for self reference
my $script_name = 'bindgraph';
# path of the RRD database
my $rrd = '/var/lib/bindgraph/bindgraph.rrd';
# temporary directory where the images will be saved
my $tmp_dir = '/var/cache/bindgraph';

my $xpoints = 620;
my $ypoints = 250;
my $rows = 24 * 60 * 7;

# HTML tags to e.g. load a style sheet or force automatic refresh
my $htmlheader = '';
# IMG tag attributes. e.g. 'width=717 height=474'
my $imgsize = '';

my $cache_time = 60;

my @graphs = (
	{ title => 'Last Hours Graph',	seconds => 3600*5,		},
	{ title => 'Day Graph',			seconds => 3600*24,		},
	{ title => 'Week Graph',		seconds => 3600*24*7,	},
	{ title => 'Month Graph',		seconds => 3600*24*31,	},
	{ title => 'Year Graph',		seconds => 3600*24*365,	},
);

my @query_t = qw(AAAA CNAME NS ANY _other_ A MX PTR SOA TKEY);
my %color = (
	MX		=> 'AA0000',
	A		=> 'FF0080',
	PTR		=> '8080C0',
	TKEY	=> '00cc00',
	CNAME	=> 'ff00ff',
	SOA		=> '00ffff',
	AAAA	=> 'ffff00',
	NS		=> 'FF8000',
	ANY		=> 'ff0000',
	_other_	=> '0000ff',
);

main();
exit 0;

##############################################################################
sub graph($$$;$) {
	my ($file, $range, $title, $small) = @_;

	my $step = $range / $rows;
	my @rrdef = map { (
		"DEF:$_=$rrd:$_:AVERAGE",
		"DEF:m$_=$rrd:$_:MAX",
		"CDEF:r$_=$_,60,*",
		"CDEF:rm$_=m$_,60,*",
		"CDEF:d$_=$_,UN,0,$_,IF,$step,*",
		"CDEF:s$_=PREV,UN,d$_,PREV,IF,d$_,+"
	) } @query_t;

	my @rrprint;
	my $stack = 0;
	foreach my $qt (@query_t) {
		# my $type = 'LINE1';
		my $type = ($stack++ == 0) ? 'AREA' : 'STACK';
		my $qts = sprintf('%7s', $qt);
		if ($small) {
			push @rrprint, "$type:$qt#" . ($color{$qt} || '000000');
		} else {
			push @rrprint, "$type:$qt#" . ($color{$qt} || '000000')
				. ":query $qts";
			push @rrprint, "GPRINT:s$qt:MAX:total\\: %8.0lf q";
			push @rrprint, "GPRINT:$qt:AVERAGE:average\\: %.2lf q/s";
			push @rrprint, "GPRINT:m$qt:MAX:max\\: %.0lf q/s\\l";
			# if you want q/m instead of q/s
			# push @rrprint, "GPRINT:s$qt:MAX:total\\: %8.0lf q";
			# push @rrprint, "GPRINT:r$qt:AVERAGE:average\\: %.2lf q/m";
			# push @rrprint, "GPRINT:rm$qt:MAX:max\\: %.0lf q/m\\l";
		}
	}

	my $date = localtime(time);
        $date =~ s|:|\\:|g unless $RRDs::VERSION < 1.199908;
	my $last = localtime(last_update($rrd));
        $last =~ s|:|\\:|g unless $RRDs::VERSION < 1.199908;

	my ($text, $xs, $ys) = RRDs::graph(
		$file,
		'--imgformat', 'PNG',
		'--width', $xpoints,
		'--height', $ypoints,
		'--start', '-' . $range,
		'--end', '-' . int($range * 0.01),
		'--vertical-label', 'queries/second',
		'--title', $title,
		'--lazy',
		@rrdef,
		@rrprint,
		'COMMENT:\s',
		'COMMENT:last update\: ' . $last . '    graph created on ' .$date . '\r',
	);
	my $err = RRDs::error;
	die_fatal("RRDs::graph($file, ...): $err") if $err;
}

sub generate_send_graph($$$;$) {
	my ($file, $range, $title, $small) = @_;

	my @sb = stat($file);
	if (not @sb or (time - $sb[9]) > $cache_time) {
		graph($file . '.tmp', $range, $title, $small);
		rename($file . '.tmp', $file) or die_fatal("rename: $!");
		@sb = stat($file);
	}

	print "Content-Type: image/png\n"
		. "Content-Length: $sb[7]\n"
		. "Cache-Control: public\n"
		. "Last-Modified: " . gmt_date($sb[9]) . "\n"
		. "Expires: " . gmt_date($sb[9] + $cache_time) . "\n"
		. "\n";

	return if $ENV{REQUEST_METHOD} eq 'HEAD';

	open(IMG, $file) or die "cannot open $file: $!";
	my $data;
	print $data while read(IMG, $data, 4096);
	close IMG;
}

sub die_fatal {
	my ($message) = @_;

	print "Content-Type: text/plain; charset=UTF-8\n\n"
		. "ERROR: $message\n";
	exit 0;
}

sub last_update {
	my ($rrd) = @_;

	my $last = RRDs::last($rrd);
	my $err = RRDs::error;
	die "RRDs::last($rrd): $err" if $err;
	return $last;
}

sub gmt_date {
	my ($when) = @_;

	my ($sec, $min, $hr, $mday, $mon, $year, $wday, $yday, $isdst)
		= gmtime($when || time);
	my $nmon = (qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec))[$mon];
	my $nday = (qw(Sun Mon Tue Wed Thu Fri Sat Sun))[$wday];
	return sprintf('%s, %02d %s %d %02d:%02d:%02d GMT',
		$nday, $mday, $nmon, $year + 1900, $hr, $min, $sec);
}

sub print_html() {
	my $page = <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>DNS Statistics for $hostname</title>$htmlheader
</head>
<body>
<h1>DNS Statistics for $hostname</h1>
HEADER

	my $script_name_real = basename $ENV{SCRIPT_NAME};
	for my $n (0 .. $#graphs) {
		$page .= "<h2>$graphs[$n]{title}</h2>\n"
			. qq#<p><img border="0" alt="bindgraph image $n" $imgsize #
			. qq#src="${script_name_real}/${script_name}_${n}.png">\n#;
	}

	# please do not remove this link from the generated page. thank you!
	$page .= <<END;
<p><a href="http://www.linux.it/~md/software/">bindgraph</a> $VERSION
by Marco Delaurenti and Marco d'Itri
</body>
</html>
END

	# the Content-Length header will enable HTTP/1.0 persistent connections
	print "Content-Type: text/html; charset=UTF-8\n"
		. "Content-Length: " . (length $page) . "\n"
#		. "Last-Modified: " . gmt_date() . "\n"
		# the validator will change for each request. this is OK (?)
		. "ETag: $$." . (time) . "\n"
		. "Expires: " . gmt_date(time + 60 * 60) . "\n"
		. "\n"
		. $page;
}

sub main {
	if (not $ENV{PATH_INFO}) {
		print_html();
		exit 0;
	}

	$ENV{REQUEST_URI} =~ m#^(.+)/[^/]+?$#; # untaint
	my $uri = $1;
	$uri =~ s#/#,#g;
	$uri =~ s#~#tilde,#g;

	die_fatal("ERROR: $tmp_dir does not exist") if not -d $tmp_dir;

	if (not -d "$tmp_dir/$uri") {
		mkdir("$tmp_dir/$uri", 0777)
			or die_fatal("ERROR: cannot create $tmp_dir: $!");
	}

	if ($ENV{PATH_INFO} !~ /^\/${script_name}_(small|\d)\.png$/) {
		die_fatal("ERROR: unknown image $ENV{PATH_INFO}");
	}

	my $file = "$tmp_dir/$uri/${script_name}_$1.png";
	if ($1 eq 'small') {
		$cache_time = 300;
		generate_send_graph($file, $graphs[0]{seconds}, $graphs[0]{title}, 1);
	} else {
		generate_send_graph($file, $graphs[$1]{seconds}, $graphs[$1]{title});
	}
}

# vim ts=4
