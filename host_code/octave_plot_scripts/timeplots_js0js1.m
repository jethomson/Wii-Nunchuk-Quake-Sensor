% Plot the time domain content of the js0 and js1 data for each axis.
% This script is useful for seeing differences between the plot of the js0 data
% and the plot of the js1 data. Place one plot window over the other and alt-tab
% between them to see the differences in the plots.

%close all
clear all
clc

filter_type = 'no filter';
%filter_type = 'moving-average M=8';
%filter_type = 'chebyshev';

load('js0_x-axis.csv')
load('js0_y-axis.csv')
load('js0_z-axis.csv')

load('js1_x-axis.csv')
load('js1_y-axis.csv')
load('js1_z-axis.csv')

% strip the first data point, because it is often invalid (i.e. 0).
js0_x_axis = js0_x_axis(2:end, :);
js0_y_axis = js0_y_axis(2:end, :);
js0_z_axis = js0_z_axis(2:end, :);
js1_x_axis = js1_x_axis(2:end, :);
js1_y_axis = js1_y_axis(2:end, :);
js1_z_axis = js1_z_axis(2:end, :);

t0ms = min([js0_x_axis(1), js0_y_axis(1), js0_z_axis(1), ...
            js1_x_axis(1), js1_y_axis(1), js1_z_axis(1)]);

% determine a good upper time axis limit so the trailing quiescent noise
% isn't shown in the plot. doing this devotes more plot area to the parts
% of the data that are interesting and makes it easier to see fine details
% when comparing plots.
[ign, m0] = min( abs(diff( flipud(js1_x_axis(:,2)(:)) ) - 5) );
xtfms = js1_x_axis(end-floor(0.75*m0),1);
[ign, m0] = min( abs(diff( flipud(js1_y_axis(:,2)(:)) ) - 5) );
ytfms = js1_y_axis(end-floor(0.75*m0),1);
[ign, m0] = min( abs(diff( flipud(js1_z_axis(:,2)(:)) ) - 10) );
ztfms = js1_z_axis(end-floor(0.75*m0),1);


% X-axis
figure('Position', [1 1 835 523]);
plot((js0_x_axis(:,1)-t0ms)/1000, js0_x_axis(:,2), 'r')
axis([0 (xtfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title(['nunchuk x-axis ' '(' filter_type ')'])

figure('Position', [1 1 835 523]);
plot((js1_x_axis(:,1)-t0ms)/1000, js1_x_axis(:,2), 'b')
axis([0 (xtfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title('jw24f8 x-axis')

% Y-axis
figure('Position', [1 1 835 523]);
plot((js0_y_axis(:,1)-t0ms)/1000, js0_y_axis(:,2), 'r')
axis([0 (ytfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title(['nunchuk y-axis ' '(' filter_type ')'])

figure('Position', [1 1 835 523]);
plot((js1_y_axis(:,1)-t0ms)/1000, js1_y_axis(:,2), 'b')
axis([0 (ytfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title('jw24f8 y-axis')

% Z-axis
figure('Position', [1 1 835 523]);
plot((js0_z_axis(:,1)-t0ms)/1000, js0_z_axis(:,2), 'r')
axis([0 (ztfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title(['nunchuk z-axis ' '(' filter_type ')'])

figure('Position', [1 1 835 523]);
plot((js1_z_axis(:,1)-t0ms)/1000, js1_z_axis(:,2), 'b')
axis([0 (ztfms-t0ms)/1000 -100 1100])
grid on
xlabel('time [s]')
ylabel('level')
title('jw24f8 z-axis')
