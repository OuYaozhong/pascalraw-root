Overview
==========

This distribution is a modification of the PASCAL VOC 2012 development kit [5] and the voc-release5 object detection framework [3]. It was developed to facilitate the training and evaluation of deformable parts models on RAW image datasets, such as PASCALRAW [2]. By using RAW images with 12-bit pixel bitdepth, it is possible to simulate the effects of hardware parameters like noise and pixel bitdepth or feature extraction techniques like logarithmic gradient extraction on object detection performance as demonstrated in [1]. 

Care was taken to make as few changes as possible to the original PASCAL VOC 2012 development kit and voc-release5 framework in order to ensure high quality model training and maintain code readability for the research community. The changes that were made to allow the use of RAW images and the simulation of hardware parameters and feature extraction techniques are summarized below.

For questions concerning the code please email <pascalrawdb AT gmail DOT com>.

Key Changes
===========

1. PASCAL VOC 2012 development kit:

   - The Training/Validation data directory, VOC2012, was replaced with VOC2014. VOC2012 contained jpeg images (.jpg) from the PASCAL VOC dataset in the JPEGImages subdirectory, whereas VOC2014 contains RAW images (.png) from the PASCALRAW dataset. Note that due to Bitbucket's 1GB repository size limit, these RAW images are not included in the repository by default, but must be downloaded separately from https://www.dropbox.com/sh/u9uiqod580scipb/AACqkfjzwNoPug7YXGPQvwgCa?dl=0. VOC2014 also replaces all of the annotation files (.xml) in the Annotations subdirectory, and all of the training set files (.txt) in the ImageSets/Main subdirectory. 

   - Lines 9 and 36 were also changed in the VOCcode/VOCinit.m file from "VOCopts.dataset='VOC2012';" to "VOCopts.dataset='VOC2014';" and from "VOCopts.imgpath=[VOCopts.datadir VOCopts.dataset '/JPEGImages/%s.jpg'];" to "VOCopts.imgpath=[VOCopts.datadir VOCopts.dataset '/JPEGImages/%s.png'];"

2. voc-release5:

   - Several small changes were made here as captured in the git revision history. The main changes include:
      - Modifying features.cc to file to simplify HOG features (See Chapter 2.5 of https://purl.stanford.edu/zd969tb2055 for details), implement logarithmic HOG features, and simulate hardware parameters such as noise, pixel and gradient bitdepth, and exposure scaling. 
      - Modifying the model configuration such that root and part filters are trained at the same scale. This would allow a model to be evaluated from features at a single scale, which may be useful for hardware implementations that cannot extract features at multiples scales simulataneously.

How to Cite
===========

If you use this code, please cite our IEEE TCSVT article and RAW image database as well as the original voc-release5 framework,
corresponding IEEE TPAMI article, and PASCAL VOC 2012 Challenge:

[1] A. Omid-Zohoor; C. Young; D. Ta; B. Murmann, "Towards Always-On Mobile Object Detection: Energy vs. Performance 
Tradeoffs for Embedded HOG Feature Extraction," in IEEE Transactions on Circuits and Systems for Video Technology, 
vol.PP, no.99, pp.1-1
doi: 10.1109/TCSVT.2017.2653187

[2] Omid-Zohoor, Alex, Ta, David, and Murmann, Boris. (2014-2015). PASCALRAW: Raw Image Database for Object Detection. Stanford Digital Repository. Available at: http://purl.stanford.edu/hq050zr7488

[3] Girshick, R. B. and Felzenszwalb, P. F. and McAllester, D., "Discriminatively Trained Deformable Part Models, 
Release 5," [Online] Available: http://people.cs.uchicago.edu/~rbg/latent-release5/

[4] P. Felzenszwalb, R. Girshick, D. McAllester, D. Ramanan. “Object Detection with Discriminatively Trained Part Based 
Models,” IEEE TPAMI, vol. 32, no. 9, September 2010.

[5] Everingham, M. and Van~Gool, L. and Williams, C. K. I. and Winn, J. and Zisserman, A., "The PASCAL Visual Object Classes Challenge 2012 (VOC2012) Results," [Online] Available: http://www.pascal-network.org/challenges/VOC/voc2012/workshop/index.html

System Requirements
===================

This code has been tested using Darwin 16.6.0 and Matlab 2012a, though other operating systems and Matlab versions should also be possible. 

How to Use
==========

Basic:

Train and evaluate a model using linear or logarithmic hog features.

1. Clone this repository to your local machine
2. Download RAW images from https://www.dropbox.com/sh/u9uiqod580scipb/AACqkfjzwNoPug7YXGPQvwgCa?dl=0 and place them in the following directory: pascalraw-root/VOC2014/VOCdevkit/VOC2014/JPEGImages/
3. Navigate to the pascalraw-root directory in the terminal, and checkout one of the following three branches:
   - 12b-linear-hog-features
   - 12b-logarithmic-hog-features
   - 2b-logarithmic-hog-features
   These branches are configured to train models with either linear or logarithmic gradients at the specified bitdepths. 
4. Make any desired changes to the Global Parameters in the features.cc file in accordance with the "Setting Global Parameters" section below.
5. Open Matlab. If not done previously, add pascalraw-root and all subfolders to path. Then train and evaluate a model by calling the pascal() function as in "pascal('car', 3);" (Note that 'car' could also be replaced with 'person' or 'bicycle' here depending on the object class of interest)
6. After the script completes, an object model (e.g. "car_final.mat") along with a number of other training and testing files will be saved to pascalraw-root/voc-release5-raw/2014/. To prevent these files from being overwritten by subsequent model training or evaluation, copy them to an external directory of choice. Copying the features.cc file used to generate the model to this directory is also recommended for future reference.

Advanced:

Evaluate a model while also simulating noise and/or imperfect exposure.

1. Copy the model of interest (e.g. "car_final.mat") to the pascalraw-root/voc-release5-raw/2014/ directory.
2. Ensure that the voc-release5 code is in the same state as when the original model was trained. Generally, this only involves replacing the pascalraw-root/voc-release5-raw/features/features.cc with the features.cc file used to train the model of interest.
3. If simulating noise, set the Global Parameter NOISE to 1 and the Global Parameter NOISE_BITDEPTH to the desired value in features.cc.
4. If simulating imperfect exposure, call the pascal_eval_test_imperfect_exposure() function as in "pascal_eval_test_imperfect_exposure('car', 2014)" (Note that 'car' could also be replaced with 'person' or 'bicycle' here depending on the object class you are interested in detecting). If not simulating imperfect exposure, call the pascal_eval_test() function as in "pascal_eval_test('car', 2014)"
6. After the script completes, a number of testing files will be saved to pascalraw-root/voc-release5-raw/2014/. To prevent these files from being overwritten by subsequent model training or evaluation, copy them to an external directory of choice. Copying the features.cc file used to generate the results to this directory is also recommended for future reference.

Setting Global Parameters:

- LOG_GRADIENTS - 0 extracts linear hog features, while 1 extracts logarithmic hog features.
- NOISE - 0 adds no noise to pixel values, while 1 adds noise to pixel values based on NOISE_BITDEPTH. Note that NOISE should be set to 0 for all model training, and only set to 1 during model evaluation, as described above in "Advanced". 
- IMAGE_BITDEPTH - Before gradient extraction, the original RAW images, are first reduced from their original bitdepth (typically 12b) to the value set by this parameter. Note that this will be true regardless of the value set for LOG_GRADIENTS. 
- NOISE_BITDEPTH - Determines the level of noise added to pixel values. 
- LOG_GRADIENT_BITDEPTH - Sets bitdepth to which logarithmic gradients are quantized for logarithmic hog features. Note that if you wish to train a model using 2b logarithmic hog features, you should check out the 2b-logarithmic-hog-features branch. Checking out the 12b-logarithmic-hog-features branch and changing the LOG_GRADIENT_BITDEPTH parameter to 2 will introduce training errors. Similarly, setting the LOG_GRADIENT_BITDEPTH parameter to anything other than 2 in the 2b-logarithmic-hog-features branch will also introduce training errors. This is due to the fact that the voc-release5 computes features for flipped images by calling flipfeat.m to mirror features. However 2b logarithmic features generate gradients that only map to 4 of the 9 unsigned orientation bins, and calling flipfeat.m actually results in features with energy in some of the orientation bins that should not be reachable. The 2b-logarithmic-hog-features branch addresses this by modifying flipfeat.m accordingly.  




