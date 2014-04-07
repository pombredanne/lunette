//
//   Copyright 2014 Vinzenz Feenstra, Red Hat Inc.
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
#include "internal.inl"
#include <lunette/registry/enum_values.hpp>

namespace lunette {
    namespace registry {
        constexpr size_t DEFAULT_NAME_BUFFER_SIZE   = 2048;
        constexpr size_t DEFAULT_VALUE_BUFFER_SIZE  = 4096;

        namespace {
            struct enum_value_context {
                std::vector<char_type> name_buffer;
                std::vector<uint8_t> value_buffer;
                DWORD name_buffer_size;
                DWORD value_buffer_size;
                DWORD value_type;
            };

            inline void clear(enum_value_context & ctx) {
                ctx.name_buffer.clear();
                ctx.name_buffer.resize(DEFAULT_NAME_BUFFER_SIZE, 0);
                ctx.name_buffer_size = ctx.name_buffer.size();

                ctx.value_buffer.clear();
                ctx.value_buffer.resize(DEFAULT_VALUE_BUFFER_SIZE, 0);
                ctx.value_buffer_size = ctx.value_buffer.size();

                ctx.value_type = REG_NONE;
            }

            inline void resize(enum_value_context & ctx) {
                if(ctx.name_buffer_size > ctx.name_buffer.size()) {
                    ctx.name_buffer.clear();
                    ctx.name_buffer.resize(ctx.name_buffer_size + 1, 0);
                    ctx.name_buffer_size = ctx.name_buffer.size();
                }
                if(ctx.value_buffer_size > ctx.value_buffer.size()) {
                    ctx.value_buffer.clear();
                    ctx.value_buffer.resize(ctx.value_buffer_size + 1, 0);
                    ctx.value_buffer_size = ctx.value_buffer.size();
                }
            }

            LONG enum_item(handle const & h, DWORD index, enum_value_context & ctx) {
                return RegEnumValue(
                    key(h),
                    index,
                    &ctx.name_buffer[0],
                    &ctx.name_buffer_size,
                    0,
                    &ctx.value_type,
                    &ctx.value_buffer[0],
                    &ctx.value_buffer_size);
            }
        }

        bool enum_values(handle const & h,
                         enum_values_callback callback) {
            DWORD number_of_values = 0;
            enum_value_context ctx;
            bool result = false;
            if( RegQueryInfoKey(key(h), 0, 0, 0, 0, 0, 0, &number_of_values, 0, 0, 0, 0) == ERROR_SUCCESS ) {
                result = true;

                for( DWORD index = 0; index < number_of_values; ++index) {
                    clear(ctx);
                    LONG success = ERROR_SUCCESS;
                    while( (success = enum_item(h, index, ctx)) == ERROR_MORE_DATA) {
                        resize(ctx);
                    }
                    if( success == ERROR_SUCCESS ) {
                        value reg_value;
                        convert_value(reg_value,
                                      &ctx.value_buffer[0],
                                      ctx.value_buffer_size,
                                      ctx.value_type);

                        ctx.name_buffer.resize(ctx.name_buffer_size);
                        lunette::string reg_name(ctx.name_buffer.begin(),
                                                 ctx.name_buffer.end());
                        callback(reg_name, reg_value);
                    }
                }
            }
            return result;
        }

        bool enum_values(registry::root root,
                         lunette::string const & path,
                         enum_values_callback callback) {
            return enum_values(handle(root, path), callback);
        }

        // Non-callback based versions
        std::map<lunette::string, value> enum_values(handle const & h) {
            std::map<lunette::string, value> result;
            enum_values(
                h,
                [&result](lunette::string const & name, value const & v){
                    result.emplace(name, v);
                    return enum_result::cont;
            });
            return std::move(result);
        }

        std::map<lunette::string, value> enum_values(registry::root root,
                                                     lunette::string const & path) {
            return std::move(enum_values(handle(root, path)));
        }
    }
}

