% Save Log
diary ~/mix-ps-gen.log

tic;
clear
close all
clc

% Pre-define the parameter
path_im = "/data/home/mb95563/pascalraw-root/VOC2014/VOCdevkit/VOC2014/JPEGImages";
path_im_mix = "/data/home/mb95563/PASCALRAW/processed/mix%d";
path_im_ps = "/data/home/mb95563/PASCALRAW/processed/ps_fsize%d";
mix_th = 2000;
ps_fsize = 3;
mix_enable = true;
ps_enable = true;

p = gcp

% Add path
% addpath(genpath(path_im));
% addpath(genpath(path_addons));

% Validate paths
    % check path_im_mix
path_tmp = sprintf(path_im_mix,mix_th);
if exist(path_tmp) ~= 7
    error(sprintf('The path_im_mix: %s, is NOT existing, or it is NOT a folder.',path_tmp));
else
    tmp = dir(path_tmp);
    if length(tmp) > 2
        error(sprintf('The path_im_mix: %s, is NOT empty. If continue, the content will be COVERED by new one.',path_tmp));
    end
end
    % check path_im_mix
path_tmp = path_im;
if exist(path_tmp) ~= 7
    error(sprintf('The path_im: %s, is NOT existing. or is NOT a folder.',path_tmp));
end
    % check path_im_ps
path_tmp = sprintf(path_im_ps,ps_fsize);
if exist(path_tmp) ~= 7
    error(sprintf('The path_im_ps: %s, is NOT existing. or is NOT a folder.',path_tmp));
else
    tmp = dir(path_tmp);
    if length(tmp) > 2
        error(sprintf('The path_im_ps: %s, is NOT empty. If continue, the content will be COVERED by new one.',path_tmp));
    end
end

% Get file list
%-----Get Image Filename & Validate
im_list = dir(path_im);
im_name = {im_list.name};
name_check = startsWith(im_name,'2014_00');
im_name = im_name(name_check);
name_check = endsWith(im_name,'.png');
im_name = im_name(name_check);
im_name = cellfun(@(x) x(1:end-4),im_name,'UniformOutput',false);
%-------------------------------

len = length(im_name);
parfor_progress(len);
parfor i = 1 : len
    I = imread(fullfile(path_im,[im_name{i} '.png']));
    % MIX TH = 2000
    I_log = uint16(log(single(I)));
    Tag = (I >= mix_th);
    I_mix = I_log .* uint16(Tag) + I .* uint16(~Tag);
    % PS
    h = ones(ps_fsize);
    h = h ./ sum(h(:));
    I_ps = imfilter(I,h,'replicate');
    imwrite(I_mix,fullfile(sprintf(path_im_mix,mix_th),[im_name{i} '.png']));
    imwrite(I_ps,fullfile(sprintf(path_im_ps,ps_fsize),[im_name{i} '.png']));
    parfor_progress;
end
parfor_progress(0);
time_cons = toc;
disp(sprintf('The process consume %f min(s).',time_cons/60));