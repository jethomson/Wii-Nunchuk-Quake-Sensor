%FILLIN - Fills in missing joystick values using the preceding neighbor.
%
% The Linux joystick driver only outputs a joystick value when it differs from
% the previous value. This leads to irregular intervals between joystick data
% points. The fft algorithm requires regularly sampled data, so fillin creates
% an array of time values with an interval of 8 milliseconds and fills in any
% missing joystick data with its preceding neighbor.
%
% Syntax:  [ti, xi] = fillin(tx_data)
%
% Inputs:
%    tx_data - matrix of time and joystick value arrays. column 1 is time values
%              and column 2 is joystick values.
%
% Outputs:
%    ti - regularly spaced time array (8 ms interval)
%    xi - joystick values with the missing values filled in.
%
% Example:
%    % the gaps are filled in with (16, 1) and (40, 5)
%    tx_data = [8*[0 1 3 4 6]; 1 1 2 5 9].'
%    [ti, xi] = fillin(tx_data)
%
% Other m-files required: none
% Subfunctions: none
% MAT-files required: none
%
% See also: NONE
%
% Author: Jonathan Thomson
% Work:
% email:
% Website: http://jethomson.wordpress.com
%

function [ti, xi] = fillin(tx_data)

	t = tx_data(:, 1);
	x = tx_data(:, 2);

	ti = (t(1):8:t(end)).';

	nt = size(ti, 1);
	range = 1:nt;
	xi = zeros(nt, 1);
	idx = lookup(t, ti);
	xi(range,:) = x(idx,:);

