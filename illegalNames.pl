
while (<>)
{
	push @libs, split /\s+/;
}

for (@libs)
{
	/(\w+)$/;
	print "\t\"$1\",\n";
}
