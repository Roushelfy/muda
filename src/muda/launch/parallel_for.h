#pragma once
#include <muda/launch/launch_base.h>
#include <muda/launch/kernel_tag.h>
#include <stdexcept>
#include <exception>

namespace muda
{
namespace details
{
    template <typename F>
    class ParallelForCallable
    {
      public:
        F   callable;
        int count;
        template <typename U>
        MUDA_GENERIC ParallelForCallable(U&& callable, int count) MUDA_NOEXCEPT
            : callable(std::forward<U>(callable)),
              count(count)
        {
        }
        // MUDA_GENERIC ~ParallelForCallable() = default;
    };

    template <typename F, typename UserTag>
    MUDA_GLOBAL void parallel_for_kernel(ParallelForCallable<F> f);

    template <typename F, typename UserTag>
    MUDA_GLOBAL void grid_stride_loop_kernel(ParallelForCallable<F> f);
}  // namespace details

enum class ParallelForType : uint32_t
{
    DynamicBlocks,
    GridStrideLoop
};

class ParallelForDetails
{
  public:
    MUDA_NODISCARD MUDA_DEVICE int  active_num_in_block() const MUDA_NOEXCEPT;
    MUDA_NODISCARD MUDA_DEVICE bool is_final_block() const MUDA_NOEXCEPT;
    MUDA_NODISCARD MUDA_DEVICE auto parallel_for_type() const MUDA_NOEXCEPT
    {
        return m_type;
    }

    MUDA_NODISCARD MUDA_DEVICE int total_num() const MUDA_NOEXCEPT
    {
        return m_total_num;
    }
    MUDA_NODISCARD MUDA_DEVICE operator int() const MUDA_NOEXCEPT
    {
        return m_current_i;
    }

    MUDA_NODISCARD MUDA_DEVICE int i() const MUDA_NOEXCEPT
    {
        return m_current_i;
    }

    MUDA_NODISCARD MUDA_DEVICE int batch_i() const MUDA_NOEXCEPT
    {
        return m_batch_i;
    }

    MUDA_NODISCARD MUDA_DEVICE int total_batch() const MUDA_NOEXCEPT
    {
        return m_total_batch;
    }

  private:
    template <typename F, typename UserTag>
    friend MUDA_GLOBAL void details::parallel_for_kernel(ParallelForCallable<F> f);

    template <typename F, typename UserTag>
    friend MUDA_GLOBAL void details::grid_stride_loop_kernel(ParallelForCallable<F> f);

    MUDA_DEVICE ParallelForDetails(ParallelForType type, int i, int total_num) MUDA_NOEXCEPT
        : m_type(type),
          m_total_num(total_num),
          m_current_i(i)
    {
    }

    ParallelForType m_type;
    int             m_total_num;
    int             m_total_batch         = 1;
    int             m_batch_i             = 0;
    int             m_active_num_in_block = 0;
    int             m_current_i           = 0;
};

using details::grid_stride_loop_kernel;
using details::parallel_for_kernel;

/// <summary>
/// ParallelFor
/// usage:
///		ParallelFor(16)
///			.apply(16, [=] __device__(int i) mutable { printf("var=%d, i = %d\n");}, true);
/// </summary>
class ParallelFor : public LaunchBase<ParallelFor>
{
    int    m_grid_dim;
    int    m_block_dim;
    size_t m_shared_mem_size;

  public:
    template <typename F>
    using NodeParms = KernelNodeParms<details::ParallelForCallable<raw_type_t<F>>>;
    /// <summary>
    /// calculate grid dim automatically to cover the range, 
    /// block size will be calculated automatically
    /// </summary>
    /// <param name="sharedMemSize"></param>
    /// <param name="stream"></param>
    MUDA_HOST ParallelFor(size_t shared_mem_size = 0, cudaStream_t stream = nullptr) MUDA_NOEXCEPT
        : LaunchBase(stream),
          m_grid_dim(0),
          m_block_dim(-1),
          m_shared_mem_size(shared_mem_size)
    {
    }


    /// <summary>
    /// calculate grid dim automatically to cover the range
    /// </summary>
    /// <param name="blockDim">block dim to use</param>
    /// <param name="sharedMemSize"></param>
    /// <param name="stream"></param>
    MUDA_HOST ParallelFor(int blockDim, size_t shared_mem_size = 0, cudaStream_t stream = nullptr) MUDA_NOEXCEPT
        : LaunchBase(stream),
          m_grid_dim(0),
          m_block_dim(blockDim),
          m_shared_mem_size(shared_mem_size)
    {
    }

    /// <summary>
    /// use Grid-Stride Loops to cover the range
    /// </summary>
    /// <param name="blockDim"></param>
    /// <param name="gridDim"></param>
    /// <param name="sharedMemSize"></param>
    /// <param name="stream"></param>
    MUDA_HOST ParallelFor(int          gridDim,
                          int          blockDim,
                          size_t       shared_mem_size = 0,
                          cudaStream_t stream          = nullptr) MUDA_NOEXCEPT
        : LaunchBase(stream),
          m_grid_dim(gridDim),
          m_block_dim(blockDim),
          m_shared_mem_size(shared_mem_size)
    {
    }

    template <typename F, typename UserTag = Default>
    MUDA_HOST ParallelFor& apply(int count, F&& f);

    template <typename F, typename UserTag = Default>
    MUDA_HOST ParallelFor& apply(int count, F&& f, Tag<UserTag>);


    template <typename F, typename UserTag = Default>
    MUDA_HOST MUDA_NODISCARD auto as_node_parms(int count, F&& f) -> S<NodeParms<F>>;

    template <typename F, typename UserTag = Default>
    MUDA_HOST MUDA_NODISCARD auto as_node_parms(int count, F&& f, Tag<UserTag>)
        -> S<NodeParms<F>>;

    MUDA_GENERIC MUDA_NODISCARD static int round_up_blocks(int count, int block_dim) MUDA_NOEXCEPT
    {
        return (count + block_dim - 1) / block_dim;
    }

  public:
    template <typename F, typename UserTag>
    MUDA_HOST void invoke(int count, F&& f);

    template <typename F, typename UserTag>
    MUDA_GENERIC int calculate_block_dim(int count) const MUDA_NOEXCEPT;

    MUDA_GENERIC int calculate_grid_dim(int count) const MUDA_NOEXCEPT;

    static MUDA_GENERIC int calculate_grid_dim(int count, int block_dim) MUDA_NOEXCEPT;

    MUDA_GENERIC void check_input(int count) const MUDA_NOEXCEPT;
};
}  // namespace muda

#include "details/parallel_for.inl"