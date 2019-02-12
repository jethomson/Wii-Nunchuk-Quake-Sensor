% Plot the frequency domain content of the js0 and js1 data for each axis.
% This script is useful for seeing differences between the plot of the js0 data
% and the plot of the js1 data. Place one plot window over the other and alt-tab
% between them to see the differences in the plots.


%close all
clear all
clc

filter_type = 'no filter';
%filter_type = 'moving-average M=8';
%filter_type = 'chebyshev';

%acc_axis_list = ['x', 'y', 'z'];
acc_axis_list = 'x';

for acc_axis = acc_axis_list

	%%% js0 %%%
	load(['js0_' acc_axis '-axis.csv'])

	% the first value is discarded because it is usually zero.
	% then fill in the gaps with the nearest preceding neighbor.
	eval(['[tt, xo] = fillin(js0_' acc_axis '_axis(2:end,:));']);

	N = 2^nextpow2(50*length(xo));

	mxo = mean(xo);
	bl = blackman(length(xo));
	[Xo, wo] = freqz(bl.*(xo-mxo), 1, N, 125);
	Xo(1) = NaN;

	figure('Position', [1 1 835 523]);
	plot(wo, 20*log10(abs(Xo)), 'r')
	a = axis();
	grid on
	xlabel('frequency [Hz]')
	ylabel('dB')
	title(['nunchuk ' acc_axis '-axis ' '(' filter_type ')'])

	%%% js1 %%%
	load(['js1_' acc_axis '-axis.csv'])

	% the first value is discarded because it is usually zero.
	% then fill in the gaps with the nearest preceding neighbor.
	eval(['[tt, xo] = fillin(js1_' acc_axis '_axis(2:end,:));']);

	N = 2^nextpow2(50*length(xo));

	mxo = mean(xo);
	bl = blackman(length(xo));
	[Xo, wo] = freqz(bl.*(xo-mxo), 1, N, 125);
	Xo(1) = NaN;

	figure('Position', [1 1 835 523]);
	plot(wo, 20*log10(abs(Xo)), 'b')
	axis(a);
	grid on
	xlabel('frequency [Hz]')
	ylabel('dB')
	title(['jw24f8 ' acc_axis '-axis '])

end
