% Simple script for plotting joystick data.

load('js0_x-axis.csv')
load('js0_y-axis.csv')
load('js0_z-axis.csv')

% strip the first data point, because it is often invalid (i.e. 0).
js0_x_axis = js0_x_axis(2:end, :);
js0_y_axis = js0_y_axis(2:end, :);
js0_z_axis = js0_z_axis(2:end, :);

t0ms = min([js0_x_axis(1), js0_y_axis(1), js0_z_axis(1)]);

figure, hold on
plot((js0_x_axis(:,1)-t0ms)/1000, js0_x_axis(:,2), 'r')
plot((js0_y_axis(:,1)-t0ms)/1000, js0_y_axis(:,2), 'g')
plot((js0_z_axis(:,1)-t0ms)/1000, js0_z_axis(:,2), 'b')

