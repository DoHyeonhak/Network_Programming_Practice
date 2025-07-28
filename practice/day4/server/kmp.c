/*
	https://chanhuiseok.github.io/posts/algo-14/	
    
    문자열 검색을 위해서는 주로 trie structure or kmp를 사용
    그러나 문자열 검색 및 포함 상태(문자열 중간에도 가능해야함)에 대하여 suffix trie로 구현하기 복잡했음
    strstr()은 bruteforce 방식을 사용하므로, 이보다 조금 더 나은 kmp 알고리즘을 사용해보기
*/

#include "kmp.h"

int* createTable(char* string, int stringLen){

	int j;
	int *table = (int*)malloc(sizeof(int) * (stringLen + 1));
	table[0] = -1;

	for(int i = 1; i < stringLen + 1; i++){
		table[i] = 0;
    }

	j = 0;

	for(int k = 1; k < stringLen; k++){
		while(string[j] != string[k] && j > 0){
			j = table[j];
		}
		if(string[j] == string[k]){
			table[k+1] = ++j;
		}
	}

	return table;
}

int kmp(char *string, char *pattern){

	int* table;
	int stringLen = strlen(string);
	int patternLen = strlen(pattern);
	int distance = 0, idx = 0, cnt = 0;
	int findFlag = 0;

    table = createTable(pattern, patternLen);

	while(1){
		idx = 0;

		if((idx + distance) + patternLen > stringLen)  // 허용 범위를 벗어나면 반복문 나가기
			break;

		while(string[idx+distance] == pattern[cnt]){   // 문자가 같을 때
			cnt++;
			idx++;

			if(cnt == patternLen){
				// printf("%d!\n", distance+1);
				findFlag = 1;
				break;
			}
		}

		distance = distance + (cnt - table[cnt]);  // 이동거리 계산
		cnt = 0;  // 일치 갯수 초기화
	}

    free(table);

    return findFlag;    // 찾았으면 1, 아니면 0
}
