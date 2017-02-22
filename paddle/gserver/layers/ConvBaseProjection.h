/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "Projection.h"
#include "paddle/math/MathUtils.h"

namespace paddle {

/**
 * @brief Base class for ConvProjection and ConvTransProjection.
 */
class ConvBaseProjection : public Projection {
public:
  /**
   * Constructor.
   */
  ConvBaseProjection(const ProjectionConfig& config,
                     ParameterPtr parameter,
                     bool useGpu);

  ~ConvBaseProjection();

protected:
  void getConvParams();
  void initCudnn();

  void reshapeTensorDesc(int batchSize);
  void reshape(int batchSize);

  size_t calOutputSize() {
    if (isDeconv_) {
      outputH_ = in_->getFrameHeight();
      outputW_ = in_->getFrameWidth();
      if (outputH_ == 0) outputH_ = configOutH_;
      if (outputW_ == 0) outputW_ = configOutW_;
      imageH_ = imageSize(outputH_,
                          filterH_,
                          paddingH_,
                          strideH_,
                          /* caffeMode */ true);

      imageW_ = imageSize(outputW_,
                          filterW_,
                          paddingW_,
                          strideW_,
                          /* caffeMode */ true);

      const_cast<Argument*>(out_)->setFrameHeight(imageH_);
      const_cast<Argument*>(out_)->setFrameWidth(imageW_);

      inputOffset_ = (configChannels_ / groups_) * outputH_ * outputW_;
      outputOffset_ = (configNumFilters_ / groups_) * imageH_ * imageW_;
      return imageH_ * imageW_ * configNumFilters_;
    } else {
      imageH_ = in_->getFrameHeight();
      imageW_ = in_->getFrameWidth();
      if (imageH_ == 0) imageH_ = configImgH_;
      if (imageW_ == 0) imageW_ = configImgW_;
      outputH_ = outputSize(imageH_,
                            filterH_,
                            paddingH_,
                            strideH_,
                            /* caffeMode */ true);
      outputW_ = outputSize(imageW_,
                            filterW_,
                            paddingW_,
                            strideW_,
                            /* caffeMode */ true);

      const_cast<Argument*>(out_)->setFrameHeight(outputH_);
      const_cast<Argument*>(out_)->setFrameWidth(outputW_);

      inputOffset_ = (configChannels_ / groups_) * imageH_ * imageW_;
      outputOffset_ = (configNumFilters_ / groups_) * outputH_ * outputW_;
      return outputH_ * outputW_ * configNumFilters_;
    }
  }

  static void* getSpaceBytes(size_t size);

  /// True if it's deconv projection layer, false if it's ConvProjection layer
  bool isDeconv_;
  /// imageH_ and imageW_ / outputH_ and outputW_
  /// is calculated from the input layer.
  int imageH_, imageW_;
  int outputH_, outputW_;
  /// configImgH_ and configImgW_ / configOutH_ and configOutW_
  /// is obtained from config.
  int configImgH_, configImgW_;
  int configOutH_, configOutW_;
  /// channels_ and numFilters_ are defined in terms of convolution semantics
  int channels_, numFilters_;
  /// configChannels and configNumFilters_ are obtained from config
  /// For Conv they are the same as channels_ and numFilters
  /// For ConvTrans they are opposite to channels_ and numFilters
  int configChannels_, configNumFilters_;
  int paddingH_, paddingW_;
  int strideH_, strideW_;
  int filterH_, filterW_;
  /// One group offset of input data.
  int inputOffset_;
  /// One group offset of output data.
  int outputOffset_;
  /// One group offset of weight.
  int weightOffset_;
  int groups_;

  /// Cudnn tensor descriptor for input.
  hl_tensor_descriptor imageDesc_;
  /// Cudnn tensor descriptor for output.
  hl_tensor_descriptor outputDesc_;
  /// Cudnn tensor descriptor for filter.
  hl_filter_descriptor filterDesc_;
  /// Cudnn tensor descriptor for a convolution operation.
  hl_convolution_descriptor convDesc_;

  /// Record the algorithm for forward convolution, which is obtained by cudnn
  /// api to search the best suited algorithm.
  int fwdAlgo_;
  /// Record the algorithm for computing convolution gradient with respect to
  /// filter coefficients.
  int bwdFilterAlgo_;
  /// Record the algorithm for computing convolution gradient with respect to
  /// the output.
  int bwdDataAlgo_;
  /// Amount of GPU memory needed as workspace to be able to execute a
  /// forward convolution with the specified algo.
  size_t fwdLimitBytes_;
  /// Amount of GPU memory needed as workspace to be able to execute a
  /// backwardFilter with the specified algo.
  size_t bwdDataLimitBytes_;
  /// Amount of GPU memory needed as workspace to be able to execute a
  /// backwardData with the specified algo.
  size_t bwdFilterLimitBytes_;
  /// Size of total work space.
  size_t workSpaceInBytes_;

  /// Whether to call cuDNN api to choose conv algorithm.
  bool isSelectAlgo_;
  /// batchNum is used to record batch size. If the batch size is changed,
  /// the selection algorithm will be called.
  int batchNum_;
  bool bias_;

  std::unique_ptr<Weight> weight_;
  static ThreadLocalD<std::vector<MemoryHandle*>> convMem_;
};

}  // namespace paddle
