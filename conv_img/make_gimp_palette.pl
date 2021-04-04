#!/usr/bin/perl

use strict;
use warnings;

sub conv_component
{
    my $v = shift;
    $v &= 0x3;
    return ($v << 6) | ($v << 4) | ($v << 2) | ($v << 0);
}

printf("GIMP Palette\n");
printf("Name: VGA Game\n");
printf("#\n");
my $index = 0;
for (my $r = 0; $r < 4; $r++) {
    for (my $g = 0; $g < 4; $g++) {
        for (my $b = 0; $b < 4; $b++) {
            printf("%3d %3d %3d Index %d\n", conv_component($r), conv_component($g), conv_component($b), $index++);
        }
    }
}
