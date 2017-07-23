function pascal_eval_test(cls, testyear)
% Evaluate a model. 
%   pascal(cls, testyear)
%
% Arguments
%   cls           Object class to train and evaluate
%   testyear      Test set year (e.g., '2007', '2011')

startup;
compile;

conf = voc_config();
cachedir = conf.paths.model_dir;
testset = conf.eval.test_set;

matlabpool close force local;
matlabpool open 12;

% TODO: should save entire code used for this run
% Take the code, zip it into an archive named by date
% print the name of the code archive to the log file
% add the code name to the training note
timestamp = datestr(datevec(now()), 'dd.mmm.yyyy:HH.MM.SS');

dotrainval = false;

note = timestamp;

testyear = conf.pascal.year;

% Record a log of the training and test procedure
diary(conf.training.log([cls '-' timestamp]));

%load model
load(strcat(sprintf('./%s/', testyear), cls, '_final.mat'))  

% Lower threshold to get high recall
model.thresh = min(conf.eval.max_thresh, model.thresh);
model.interval = conf.eval.interval; 

suffix = testyear;

% Collect detections on the test set
ds = pascal_test(model, testset, testyear, suffix);

% Evaluate the model without bounding box prediction
ap1 = pascal_eval(cls, ds, testset, testyear, suffix);
fprintf('AP = %.4f (without bounding box prediction)\n', ap1)

% Recompute AP after applying bounding box prediction
[ap1, ap2] = bboxpred_rescore(cls, testset, testyear, suffix);
fprintf('AP = %.4f (without bounding box prediction)\n', ap1)
fprintf('AP = %.4f (with bounding box prediction)\n', ap2)

% Compute detections on the trainval dataset (used for context rescoring)
if dotrainval
  trainval(cls);
end
