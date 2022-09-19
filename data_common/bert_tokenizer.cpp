#include "bert_tokenizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>


namespace lazydog {

    /**
     * @brief 
     * unicode value of all chinese punc chars...
     */
    std::set<wchar_t> BertTokenizer::chinese_punc_chars={
        8211, 8212, 8216, 8217, 8220, 8221, 
        8230, 12289, 12290, 12296, 12297, 12298, 
        12299, 12300, 12301, 12302, 12303, 12304, 
        12305, 12308, 12309, 65281, 65288, 65289, 
        65292, 65294, 65306, 65307, 65311
    };

    BertTokenizer::BertTokenizer(){
        vocab_path="vocab.txt";
    };

    void BertTokenizer::print_list_string(std::list<std::wstring> &text_list) {
        for (auto iter = text_list.begin(); iter != text_list.end(); ++iter) {
            std::cout << utf8_converter.to_bytes(*iter) << "->";
        }
        std::cout << std::endl;
    }

    BertTokenizer::BertTokenizer(std::string vocab_path_):vocab_path(vocab_path_){};

    BertTokenizer::BertTokenizer(std::string vocab_path_, uint32_t max_input_chars_per_word_):
        vocab_path(vocab_path_),max_input_chars_per_word(max_input_chars_per_word_){};

    BertTokenizer::BertTokenizer(std::string vocab_path_, std::wstring sep_token_,
                                 std::wstring mask_token_, std::wstring pad_token_, std::wstring cls_token_,
                                 uint32_t max_input_chars_per_word_):
        vocab_path(vocab_path_),sep_token(sep_token_),mask_token(mask_token_),pad_token(pad_token_),
        cls_token(cls_token_),max_input_chars_per_word(max_input_chars_per_word_){};

    uint32_t BertTokenizer::convert_token_2_id(std::wstring& token){
        return token_2_id[token];
    }

    /**
     * @brief 
     * convert all the tokens -> ids,return a vector which has same size!
     * @param tokens 
     * @return std::vector<uint32_t> 
     */
    std::vector<uint32_t> BertTokenizer::convert_tokens_2_ids(std::list<std::wstring>& tokens){
        // printf("fuck!\n");
        size_t token_size = tokens.size();
        std::vector<uint32_t> token_ids(token_size,0);
        uint64_t i=0;
        for(auto iter=tokens.begin();iter!=tokens.end();++iter){
            token_ids[i++] = token_2_id[*iter];
        }
        return token_ids;
    }

    /**
     * @brief 
     * convert tokens -> ids,but specify a max_size,we will compare and use min(max_size,token_size as the vector size!
     * @param tokens 
     * @param max_size 
     * @return std::vector<uint32_t> 
     */
    std::vector<uint32_t> BertTokenizer::convert_tokens_2_ids(std::list<std::wstring>& tokens,size_t max_size){
        max_size = tokens.size() > max_size?max_size:tokens.size();
        std::vector<uint32_t> token_ids(max_size,0);
        auto iter = tokens.cbegin();
        for(size_t i=0;i<max_size;++i){
            token_ids[i] = token_2_id[*iter];
            ++iter;
        }
        return token_ids;
    }

    /**
     * @brief 
     * we will write the ids -> specify input_ids!
     * @param tokens 
     * @param input_ids 
     */
    void BertTokenizer::convert_tokens_2_ids(std::list<std::wstring> &tokens, std::vector<uint32_t> &input_ids){
        uint32_t max_size = tokens.size() > input_ids.size() ? input_ids.size():tokens.size() ;
        int i = 0;
        auto iter = tokens.cbegin();
        for (uint32_t i=0;i<max_size;++i) {
            input_ids[i] = token_2_id[*iter];
            ++iter;
        }
    }

    // check the sizeof tokens equals to input_ids
    void BertTokenizer::convert_tokens_2_ids_with_check(std::list<std::wstring>& tokens,std::vector<uint32_t>& input_ids){
        if(tokens.size() != input_ids.size()){
            printf("token size mismatch with input_id size!we can not continue\n");
            return;
        }
        int i = 0;
        for (auto iter = tokens.begin(); iter != tokens.end(); ++iter) {
            input_ids[i] = token_2_id[*iter];
            ++i;
        }
    }

    void BertTokenizer::transfer_string_to_upper(std::wstring& text){
        size_t text_size = text.size();
        for(size_t i=0;i<text_size;++i){
            if( SpecialAscii::LowerCaseEnMinimumAscii <= text[i] && text[i] >=SpecialAscii::LowerCaseEnMaximumAscii ){
                text[i] -=32;
            }
        }
    }

    void BertTokenizer::transfer_string_to_lower(std::wstring& text){
        size_t text_size = text.size();
        for(size_t i=0;i<text_size;++i){
            if(text[i] >=SpecialAscii::UpperCaseEnMinimumAscii && text[i] <=SpecialAscii::UpperCaseEnMaximumAscii){
                text[i]+=32;
            }
        }
    }

    size_t BertTokenizer::get_vocab_size() const{
        return (token_2_id.size() + added_token_2_id.size());
    }
    
    
    void BertTokenizer::add_custom_tokens(std::list<std::wstring>& add_tokens,bool is_special=true){
        printf("add new tokens to vocab!\n");
        size_t token_size = add_tokens.size();
        std::set<std::wstring> tokens_to_add;
        size_t base_vocab_size = get_vocab_size();
        for(auto iter=add_tokens.begin();iter!=add_tokens.end();++iter){
            std::wstring& new_token = (*iter);
            if(!is_special){
                transfer_string_to_lower(new_token);
            }
            bool flag_1 = (token_2_id.find(new_token) == token_2_id.end());
            bool flag_2= (tokens_to_add.find(new_token) == tokens_to_add.end());
            bool flag_3 = (added_token_2_id.find(new_token) == token_2_id.end());
            if(flag_1 && flag_2 && flag_3){
                printf("add a new token -> %s\n",utf8_converter.to_bytes(new_token).c_str());
                added_token_2_id[new_token] = base_vocab_size;
                added_id_2_token[base_vocab_size] = new_token;
                base_vocab_size ++;
            }
        }
    }

    bool BertTokenizer::_is_chinese_char(wchar_t text_char){
        bool flag = (19968<=text_char && text_char <= 40959) ||
            (13312 <= text_char && text_char <= 19903) || 
            (131072 <= text_char && text_char <= 173791) ||
            (173824 <= text_char && text_char <= 183983) ||
            (63744 <= text_char && text_char <= 64255) ||
            (194560 <= text_char && text_char <= 195103);
        return flag;
    }
    
    // how to do it!
    std::wstring BertTokenizer::_run_strip_accents(std::wstring& text){
        return text;
    }

    std::list<std::wstring> BertTokenizer::_run_split_on_punc(std::list<std::wstring>& basic_text_tokens){
        std::list<std::wstring> split_result;
        size_t start_index;
        for(auto iter=basic_text_tokens.begin();iter!=basic_text_tokens.end();++iter){
            if(iter->size() == 1){
                split_result.push_back(std::move((*iter)));
                continue;
            }
            // must be reset -> 0 for each loop!
            start_index = 0;
            for(size_t i=0;i<iter->size();++i){
                if(_is_punctuation_char((*iter)[i])){
                    if(i>start_index){
                        split_result.push_back(iter->substr(start_index, i - start_index));
                    }
                    split_result.push_back(iter->substr(i,1));
                    start_index = i+1;
                }
            }
            if(start_index < iter->size()){
                // just compute the offset!
                split_result.push_back(iter->substr(start_index,iter->size() - start_index));
            }
        }
        return split_result;
    }

    std::list<std::wstring> BertTokenizer::_whitespace_tokenize(std::wstring &text) {
        std::cout << "current string : " << utf8_converter.to_bytes(text) << std::endl;
        size_t i = 0;
        size_t j = 0;
        size_t index_bound = text.size() - 1;
        std::list<std::wstring> split_result;
        while (text[j] == SpecialAscii::WhiteSpaceAscii) {
            ++j;
            ++i;
        }
        while (text[index_bound] == SpecialAscii::WhiteSpaceAscii) {
            --index_bound;
        }
        ++j;
        for (j;j<=index_bound-1;++j) {
            if (text[j] == SpecialAscii::WhiteSpaceAscii) {
                // the whitespace maybe >1,we should filter it!
                if (j == i) {
                    ++i;
                    continue;
                }
                std::cout << "i: " << i << " j: " << j << std::endl;
                split_result.push_back(text.substr(i, (j - i)));
                i = j + 1;
            }
        }
        if(j<i){
            split_result.push_back(text.substr(i, (j - i)));
        }
        print_list_string(split_result);
        return split_result;
    }

    std::list<std::pair<std::wstring,bool>> BertTokenizer::_split_text_with_regexp_search(std::wstring& text){
        std::list<std::pair<std::wstring,bool>> split_result;
        std::wstring::const_iterator head = text.cbegin();
        std::wstring::const_iterator tail = text.cend();
        std::wsmatch search_result;
        std::wstring::difference_type _intercep_start = 0;
        while(std::regex_search(head,tail,search_result,pattern)){
            auto left = search_result[0].first;
            auto right = search_result[0].second;
            split_result.push_back({text.substr(_intercep_start,left-head),false});
            split_result.push_back({std::move(search_result[0].str()),true});
            head = right;
            _intercep_start = right - text.cbegin();
        }
        if(head!=text.cend()){
            split_result.push_back({text.substr(_intercep_start,tail-head),false});
        }
        return split_result;
    }

    void BertTokenizer::reset_vocab_path(std::string new_vocab_path,bool reset_vocab){
        vocab_path = new_vocab_path;
        if(reset_vocab){
            printf("reading new vocab file...\n");
            read_vocab_paris_from_file();
        }
    }
    void BertTokenizer::reset_max_input_chars_per_word(uint32_t new_max_input_chars_per_word){
        printf("reset [param:]max_input_chars_per_word form %d -> %d\n",max_input_chars_per_word,new_max_input_chars_per_word);
        max_input_chars_per_word = new_max_input_chars_per_word;
    }

    std::wstring BertTokenizer::join_substrs(std::list<std::wstring> substrs){
        // compute all the data size firstly!
        size_t total_size = 0;
        for(auto iter=substrs.begin();iter!=substrs.end();++iter){
            total_size += iter->size();
        }
        std::wstring join_string;
        join_string.reserve(total_size);
        for(auto iter=substrs.begin();iter!=substrs.end();++iter){
            join_string.append(*iter);
        }
        return join_string;
    }

    std::list<std::wstring> BertTokenizer::wordpiece_tokenize(std::list<std::wstring> &text_tokens) {
        std::list<std::wstring> output_tokens;
        std::cout << "**********wordpiece*************" << std::endl;
        for(auto iter=text_tokens.begin();iter!=text_tokens.end();++iter){
            auto token_size = iter->size();
            if(token_size == 1){
                // just move,to avoid memory allocate / free
                output_tokens.push_back(std::move(*iter));
                continue;
            }
            if(token_size > max_input_chars_per_word){
                output_tokens.push_back(unk_token);
                continue;
            }
            bool is_bad = false;
            size_t start = 0;
            size_t end;
            std::wstring current_substr;
            // to save the cut sub tokens...
            std::list<std::wstring> sub_tokens;
            while(start < token_size){
                end = token_size;
                while(start<end){
                    std::wstring substr = iter->substr(start,end-start);
                    if(start>0){
                        substr = std::wstring(L"##") + substr;
                    }
                    if(token_2_id.find(substr)!=token_2_id.end()){
                        current_substr = std::move(substr);
                        break;
                    }
                    --end;
                }
                if(!current_substr.size()){
                    is_bad = true;
                    break;
                }
                sub_tokens.push_back(std::move(current_substr));
                start = end;
            }
            if(is_bad){
                output_tokens.push_back(unk_token);
            }else{
                // after this,the ele of sub_tokens will be set -> nullptr!
                output_tokens.splice(output_tokens.end(),sub_tokens);
            }
        }
        return output_tokens;
    }

    std::list<std::wstring> BertTokenizer::_tokenize(std::wstring& text){
        size_t text_size = text.size();
        std::vector<uint8_t> text_unicode_flag_list(text_size,1);
        uint32_t whitespace_size = 0;
        for(uint32_t i=0;i<text_size;++i){
            if (!_is_chinese_char(text[i])){
                text_unicode_flag_list[i] = 0;
                continue;
            }
            whitespace_size += 2;
        }
        // bool text_ord_flag = std::all_of(text.begin(),text.end(),_is_chinese_char);
        // means that all of char is chinese char,only need to return each substr!
        std::list<std::wstring> tokens;
        std::cout << "whitespace_size: " << whitespace_size << "   text_size: " << text_size << std::endl;
        if(whitespace_size == 2*text_size){
            for(size_t i=0;i<text_size;++i){
                tokens.push_back(text.substr(i,1));
            }
            return tokens;
        }
        
        std::wstring first_padding_string(text_size + whitespace_size,0);
        size_t start_index = 0;
        // pad whitespace string if chinese char!,and compute the size of padding whitespace
        // judge for the first token...
        if(text_unicode_flag_list[0]){
            first_padding_string[start_index++] = text[0];
            first_padding_string[start_index++] = SpecialAscii::WhiteSpaceAscii;
        }else{
            first_padding_string[start_index++] = text[0];
        }
        for(size_t i=1;i<text_size-1;++i){
            if(text_unicode_flag_list[i]){
                first_padding_string[start_index++] = SpecialAscii::WhiteSpaceAscii;
                first_padding_string[start_index++] = text[i];
                first_padding_string[start_index++] = SpecialAscii::WhiteSpaceAscii;
            }else{
                first_padding_string[start_index++] = text[i];
            }
        }

        for(size_t i=0;i<text_size;)
        std::list<std::wstring> basic_text_tokens = _whitespace_tokenize(first_padding_string);
        std::cout << "basci_text_tokens" << std::endl;
        print_list_string(basic_text_tokens);
        std::list<std::wstring> puncated_text_tokens = _run_split_on_punc(basic_text_tokens);
        std::cout << "puncated_text_tokens" << std::endl;
        print_list_string(puncated_text_tokens);
        std::list<std::wstring> wordpiece_text_tokens = wordpiece_tokenize(puncated_text_tokens);
        std::cout << "wordpiece_text_tokens" << std::endl;
        print_list_string(wordpiece_text_tokens);
        return wordpiece_text_tokens;
    }

    std::list<std::wstring> BertTokenizer::tokenize(std::wstring& text){
        std::list<std::wstring> final_output_token;
        std::list<std::pair<std::wstring,bool>> split_subtext = _split_text_with_regexp_search(text);
        for(auto iter=split_subtext.begin();iter!=split_subtext.end();++iter){
            if(iter->second){
                std::cout << "special token " << utf8_converter.to_bytes(iter->first) << std::endl;
                final_output_token.push_back(std::move(iter->first));
            }else{
                std::cout << "sub string " << utf8_converter.to_bytes(iter->first) << std::endl;
                std::list<std::wstring> sub_tokens = _tokenize(iter->first);
                final_output_token.splice(final_output_token.end(),sub_tokens);
            }
        }
        return final_output_token;
    }

    std::list<std::wstring> BertTokenizer::tokenize_without_never_split(std::wstring& text){
        return _tokenize(text);
    }


    bool BertTokenizer::_is_punctuation_char(wchar_t text_char){
        bool flag = (33<=text_char && text_char<=47) ||
                    (58<=text_char && text_char<=64) ||
                    (91<=text_char && text_char<=96) ||
                    (123<=text_char && text_char<=126) ||
                    (chinese_punc_chars.find(text_char)!=chinese_punc_chars.end());
        return flag;
    }

    /**
     * @brief 
     * read the token from file,
     * @param line_sep 
     */
    void BertTokenizer::read_vocab_paris_from_file(){
        if (token_2_id.size() > 0){
            printf("clear the old key,value!\n");
            token_2_id.clear();
            id_2_token.clear();
        }
        std::ifstream reader(vocab_path);
        if(!reader.is_open()){
            printf("failed to open file [%s],please check the path is correct!\n",vocab_path.c_str());
            return;
        }
        std::string temp_line;
        uint32_t i = 0;
        while(std::getline(reader,temp_line)){
            std::wstring utf8_token = utf8_converter.from_bytes(temp_line);
            token_2_id[utf8_token] = i;
            id_2_token[i] = std::move(utf8_token);
            ++i;
        }
        printf("read %d tokens from vocab file\n",i);
    }

    /**
     * @brief
     * if the header and tail of line equals to line sep,we will strip the head and tail
     * and we will not check the bound,so be sure the sizeof sep less than the string you want to split!!!
     * @param line 
     * @param line_sep 
     * @return std::list<std::wstring> 
     */
    std::list<std::wstring> BertTokenizer::split_line(std::wstring& line,std::wstring& line_sep){
        size_t sep_size = line_sep.size();
        size_t text_size = line.size();
        // record the _intercept size!
        size_t start_index = 0;
        size_t i=0;
        std::list<std::wstring> split_result;
        // if line starts with line_sep,we should skip the line_sep
        if(line.substr(0,sep_size) == line_sep){
            i = i+ sep_size;
            start_index = i;
        }
        while(i<text_size){
            if(line[i] == line_sep[0] && line_sep == line.substr(i,sep_size)){
                // means that found a sep
                split_result.push_back(line.substr(start_index,i-start_index));
                i = i + sep_size;
                // plus the offset
                start_index =i;
                continue;
            }
            ++i;
        }
        // if the end of line is not line_sep,we should push back the remain substr!
        if(start_index < text_size){
            split_result.push_back(line.substr(start_index,text_size - start_index));
        }
        return split_result;
    }

    /**
     * @brief 
     * another version for split_line function,if the head and tail equals to the sep,
     * we will not check them,and maybe produce a empty string!
     * @param line 
     * @param line_sep 
     * @return std::list<std::wstring> 
     */
    std::list<std::wstring> BertTokenizer::split_line_v2(std::wstring& line,std::wstring& line_sep){
        size_t line_size = line.size();
        size_t sep_size = line_sep.size();
        if(line_size < sep_size){
            printf("bad params,the sizeof sep:%ld is greater than line size:%ld\n",line_size,sep_size);
            // empty construct!
            return {};
        }

        if (line_size == sep_size){
            printf("emm,the sizeof line equals to line sep,be sure you need do it!\n");
            return {line};
        }

        size_t start_index = 0;
        size_t i = 0;
        std::list<std::wstring> split_result;
        while (i < line_size){
            if(line[i] == line_sep[i] && line_sep == line.substr(i,sep_size)){
                split_result.push_back(line.substr(start_index,i-start_index));
                i = i + sep_size;
                start_index = i;
                continue;
            }
            ++i;
        }
        // push back the remain size!
        if(start_index < line_size){
            split_result.push_back(line.substr(start_index,i-start_index));
        }else if(start_index == line_size){
            // if the tail equals to line_sep,we will pad a empty string for it!
            split_result.push_back({});
        }
        return split_result;
    }
}