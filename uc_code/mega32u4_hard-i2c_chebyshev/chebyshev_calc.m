%
%

% low pass filter with cutoff pi*Wc radians
%Fs = 1805;
%Fs = 903;
Fs = 250;
fc = 38.725; % fc = 38.725 --> 3dB point at 50 Hz

Wc = fc/(Fs/2);

n = 4; % filter order
pr = 0.5; % percent ripple
Rp = 20*log10(100/(100-pr)); % passband ripple in dB

[B, A] = cheby1(n, Rp, Wc);

% largest round-off error is about one ten-millionth of the largest
% coefficient.
rerr = max(abs(single(A)))*10^-7;
if any(abs(single(B)) < rerr)
	disp('warning: coefficient(s) below round-off error.')
endif

% does this work?
if any( ((single(B) - B) ~= 0) || ((single(A) - A) ~= 0) )
	disp('warning: coefficients(s) cannot be represented with single precision.')
endif


format long g
fp = fopen('lpf.h', 'w');
fprintf(fp, '// Chebyshev coefficients for a %i pole low-pass filter\n', n);
fprintf(fp, '// with a 3dB freq. of 50 Hz when the sampling rate is %g\n\n', Fs);
fprintf(fp, '// filter length\n')
fprintf(fp, '#define N %u\n\n', n+1);

fprintf(fp, 'float b[N] = {')
for ii = 1:n
	fprintf(fp, '%g, ', single(B(ii)))
end
fprintf(fp, '%g};\n', single(B(end)))

fprintf(fp, 'float a[N] = {')
for ii = 1:n
	fprintf(fp, '%g, ', single(A(ii)))
end
fprintf(fp, '%g};\n', single(A(end)))

fclose(fp);

