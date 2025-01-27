#pragma once

#include "column/binary_column.h"
#include "column/column_visitor.h"
#include "column/column_visitor_adapter.h"
#include "column/fixed_length_column.h"
#include "common/status.h"
#include "common/statusor.h"
#include "runtime/primitive_type.h"
#include "udf/java/java_udf.h"

namespace starrocks::vectorized {
struct JavaUDAFState {
    JavaUDAFState(jobject&& handle_) : handle(std::move(handle_)){};
    ~JavaUDAFState() = default;
    // UDAF State
    jobject handle;
};
// Column to DirectByteBuffer, which could avoid some memory copy,
// directly access the C++ address space in Java
// Because DirectBuffer does not hold the referece of these memory,
// we must ensure that it is valid during accesses to it

class ConvertDirectBufferVistor : public ColumnVisitorAdapter<ConvertDirectBufferVistor> {
public:
    ConvertDirectBufferVistor(std::vector<DirectByteBuffer>& buffers) : ColumnVisitorAdapter(this), _buffers(buffers) {}
    Status do_visit(const NullableColumn& column);
    Status do_visit(const BinaryColumn& column);

    template <typename T>
    Status do_visit(const vectorized::FixedLengthColumn<T>& column) {
        get_buffer_data(column, &_buffers);
        return Status::OK();
    }

    template <typename T>
    Status do_visit(const T& column) {
        return Status::NotSupported("UDF Not Support Type");
    }

private:
    template <class ColumnType>
    void get_buffer_data(const ColumnType& column, std::vector<DirectByteBuffer>* buffers) {
        const auto& container = column.get_data();
        buffers->emplace_back((void*)container.data(), container.size() * sizeof(typename ColumnType::ValueType));
    }

private:
    std::vector<DirectByteBuffer>& _buffers;
};

class JavaDataTypeConverter {
public:
    static jobject convert_to_object_array(uint8_t** data, size_t offset, int num_rows);

    static void convert_to_boxed_array(FunctionContext* ctx, std::vector<DirectByteBuffer>* buffers,
                                       const Column** columns, int num_cols, int num_rows, std::vector<jobject>* res);
};
} // namespace starrocks::vectorized