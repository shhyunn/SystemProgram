#include "hpfp.h"
#include <stdlib.h>

typedef union hpfpStruct { //구조체 비트 배열 선언
	hpfp result;
	struct {
		hpfp frac : 10; //가수 10비트
		hpfp exp : 5; //지수 5비트
		hpfp sBit : 1; //부호 1비트
	};
} hpfpS;

typedef union floatStruct {
	float num;
	struct { //비트 연산 위해 int형으로 정의
		unsigned int numInt;
	};
} floatS;

hpfp int_converter(int input) {
	hpfpS hfs1; //객체 생성

	hfs1.sBit = 0; //부호 비트 0으로 초기화

	if (input < 0) { //정수값이 음수일 경우
		hfs1.sBit = 1; //부호비트 1로 설정
		input = -input; //정수값 양수로 변환
	}

	if (input == 0) { //input 값이 0일 경우
		hfs1.sBit = 0; //부호 비트 0으로 설정
		hfs1.exp = 0; //지수 비트 0으로 설정
		hfs1.frac = 0; //가수 비트 0으로 설정

		return hfs1.result;
	}

	unsigned int inputCopy = input; //복사본 만들어서 계산
	unsigned int inputCopy2 = input; 

	int cnt = 0; //0으로 초기화
	for (int i = 0; i < 32; i++) { //int는 4바이트이므로, 32까지 반복
		if (inputCopy == 0) //0이 되면 반복문 종료
			break;
		inputCopy >>= 1; //우측으로 비트시프트
		cnt += 1; //비트 자리 +1
	}

	unsigned int exp = (cnt - 1) + 15; //지수부 계산
	if (exp >= 31) {//5비트 넘어갈 경우
		hfs1.exp = 31; //무한대 예외 처리
		hfs1.frac = 0;
	}
	else {
		hfs1.exp = exp; //그대로 저장
		hfs1.frac = inputCopy2 << (10 - (cnt - 1)); // 10-실제 지수 만큼 좌측 시프트
	}
	return hfs1.result;
}

int hpfp_to_int_converter(hpfp input) {
	int intRes = 0; //부호가 있는 int형 변수 선언
	unsigned short sign = input >> 15; //부호 비트만 남기기
	unsigned short exp = input << 1; //지수부 비트만 남기기
	exp >>= 11;
	unsigned short frac = input << 6; //가수부 비트만 남기기
	frac >>= 6;

	if (exp == 31) { //지수부가 무한대일 경우
		if (frac == 0) { //가수부 비트가 0인 경우(무한대)
			if (sign == 0) {//부호 비트가 0인 경우 TMAX
				intRes = 0; 
				for (int i = 0; i < 31; i++) { //TMAX로 비트배열 만들기
					intRes <<= 1;
					intRes += 1; //01111....
				}
			}
			else if (sign == 1) {//부호 비트가 1인 경우 TMIN
				intRes = 1;
				for (int j = 0; j < 31; j++) { //TMIN으로 비트배열 만들기
					intRes <<= 1;
					intRes += 1; //1111111....
				}
			}
		}
		else if (frac != 0) { //가수부 비트 1이 하나라도 포함되어 있을 경우 (NaN) TMIN
			intRes = 1;
			for (int k = 0; k < 31; k++) { //TMIN으로 비트배열 만들기
				intRes <<= 1;
				intRes += 1;
			}
		}
	}
	else if (exp == 0 && frac == 0) { //지수부, 가수부 모두 0일 경우
		intRes = 0;
	}
	else { //지수부가 무한대가 아닐 경우
		int expReal = exp - 15; //실제 지수 구하기 위해 바이어스값 15 빼기
		if (expReal >= 0) { //0 이상인 경우 우측 시프트
			frac >>= (10 - expReal); //(10-실제 지수)만큼 가수부 우측시프트
			frac += (1 << expReal); //맨 앞에 1 붙여주기
			if (sign == 0) { //부호비트가 0인 경우 그대로 결과값에 저장
				intRes = frac;
			}
			else if (sign == 1) { //부호비트가 1인 경우 음수로 결과값 저장
				intRes = -frac;
			}
		}
		else if (expReal < 0) { //round-to-zero 적용
		intRes = 0;
		}
	}
	return intRes;

}

hpfp float_converter(float input) {
	hpfpS hfs2; //객체 생성
	floatS floatNum; //float형 비트 연산 위한 객체 생성

	floatNum.num = input; //float형으로 input값 저장
	hfs2.sBit = floatNum.numInt >> 31; //가장 왼쪽 비트 얻기 위하여 우측비트시프트
	if (input == 0) { //input 값이 0일 경우
		hfs2.exp = 0;
		hfs2.frac = 0;
		return hfs2.result;
	}
	unsigned int tempExp = floatNum.numInt << 1; //지수값 계산 위해 비트시프트
	unsigned int tempFrac = floatNum.numInt << 9; //부호비트와 지수부 비트 삭제
	tempFrac >>= 22; //10비트 남기기

	tempExp >>= 24; //8비트 남기기 위해 우측 비트시프트
	tempExp = tempExp - 127; //실제 지수 얻기 위해 32비트 바이어스 값 127 빼기

	unsigned int exp = tempExp + 15; //바이어스값 15 더하여 변수에 저장
	if (exp >= 31 && tempFrac == 0) {//31이상일 경우 무한대 처리
		hfs2.exp = 31;
		hfs2.frac = 0;
	}
	else if (exp >= 31 && tempFrac != 0) {
		hfs2.exp = 31;
		hfs2.frac = 1;
	}
	else {
		hfs2.exp = exp;
		hfs2.frac = tempFrac;
	}
	return hfs2.result;
}

float hpfp_to_float_converter(hpfp input) {
	floatS result; //float형 변수 생성
	unsigned short sign = input >> 15; //부호비트
	unsigned short exp = input << 1; //지수부 비트
	exp >>= 11;

	unsigned short frac = input << 6; //가수부 비트
	frac >>= 6;

	result.numInt = sign; //초기값 부호 비트로 설정
	if (exp == 31) { //지수부 비트가 모두 1일 경우
		for (int i = 0; i < 8; i++) { //8비트까지 반복
			result.numInt <<= 1; //좌측 시프트
			result.numInt += 1; //8비트 모두 1로 설정
		}
		if (frac == 0) { //가수부 비트가 0일 경우
			result.numInt <<= 23; //모두 0으로 채우기
		}
		else if (frac != 0) { //가수부 비트가 0이 아닐 경우
			result.numInt <<= 23;
			result.numInt += 1; //가수부 비트 0이 아닌 값으로 채우기,NaN
		}
	}
	else if (exp == 0 && frac == 0) { //지수부, 가수부 비트 모두 0일 경우
		result.num = 0;
	}
	else { //지수부 비트가 모두 1이 아닐 경우
		int expReal = exp - 15; //바이어스값 15 빼서 실제 지수값 구하기
		exp = expReal + 127; //다시 바이어스값 127 더해서 exp에 저장
		for (int k = 7; k >= 0; --k) { //지수부 비트 8비트 동안 반복
			int temp = exp >> k & 1;
			result.numInt <<= 1;
			result.numInt += temp;
		}
		for (int l = 9; l >= 0; --l) { //가수부 비트 10비트 동안 반복
			int temp2 = frac >> l & 1;
			result.numInt <<= 1;
			result.numInt += temp2;
		}
		result.numInt <<= 13; //나머지 가수부 비트 23비트 채우기 위해 13만큼 우측 비트시프트
	}
	return result.num; //float형으로 반환
}

hpfp addition_function(hpfp a, hpfp b) {
	hpfpS addRes; //결과값 객체
	unsigned int add = 1; //add 플래그 1로 초기화
	unsigned int order = 1; //순서 플래그 1로 초기화
	unsigned short signA = a >> 15; //부호비트 a
	unsigned short expA = a << 1; //지수비트 a
	expA >>= 11;
	unsigned short fracA = a << 6; //가수비트 a
	fracA >>= 6;

	unsigned short signB = b >> 15;//부호비트 b
	unsigned short expB = b << 1;//지수비트 b
	expB >>= 11;
	unsigned short fracB = b << 6; //가수비트 b
	fracB >>= 6;

	addRes.sBit = signA; //결과 부호비트 a 부호비트로 설정

	if (((expA == 31) && (fracA != 0)) || ((expB == 31) && (fracB != 0))) { //NaN이 포함되어 있을 경우
		addRes.exp = 31; //NaN으로 설정
		addRes.frac = 1;
		return addRes.result;
	}

	if (((expA == 31) && (fracA == 0)) && ((expB == 31) && (fracB == 0))) { //a,b 모두 무한대일 경우
		addRes.exp = 31;
		addRes.frac = 0;
		if (signA != signB) { //a,b의 부호가 다를 경우
			addRes.frac = 1; //NaN으로 처리
		}
		return addRes.result;
	}
	else if ((expA == 31) && (fracA == 0)) { //a만 무한대일 경우
		addRes.exp = 31;
		addRes.frac = 0;
		return addRes.result; //무한대로 처리
	}
	else if ((expB == 31) && (fracB == 0)) { //b만 무한대일 경우
		addRes.sBit = signB; //부호비트 b로 설정
		addRes.exp = 31;
		addRes.frac = 0;
		return addRes.result; //무한대로 처리
	}

	if (signA != signB) { //부호가 다를 경우
		add = 0; //가수부 뺄셈 진행
		if (expA > expB) { //a 지수가 더 클 경우
			addRes.sBit = signA; //a 부호비트로 설정
		}
		else { //b 지수가 더 클 경우
			addRes.sBit = signB; //b 부호비트로 설정
			order = 0; //b에서 a 순으로 연산 진행
		}
	}

	int expDiff = expA - expB; //지수 차 구하기
	fracA += 0x400;
	fracB += 0x400;

	if (expDiff > 0) { //a 지수가 더 클 경우
		fracB >>= expDiff; //b 가수부 우측 시프트
		expB = expA; //a 지수로 바꾸기
	}
	else if (expDiff < 0) { //b 지수가 더 클 경우
		fracA >>= -expDiff ; //a 가수부 우측 시프트
		expA = expB; //b 지수로 바꾸기
	}

	unsigned short fracT; //가수부 결과값 설정

	if (add == 1) { //더할 경우
		fracT = fracA + fracB;
	}
	else { //뺄 경우
		if (order == 1) { //a->b 순으로 연산 진행할 경우
			fracT = fracA - fracB;
		}
		else { //b->a 순으로 연산 진행할 경우
			fracT = fracB - fracA;
		}
	}

	if (((fracT & 0x800) >> 11) == 1) { //가수부 11비트 초과
		if ((fracT & 1) == 1) { //홀수인 경우
			fracT >>= 1; 
			expA += 1; 
			fracT += 1; 
		}
		else { //짝수인 경우
			expA += 1; //반올림
			fracT >>= 1;
		}
	}

	addRes.exp = expA; //지수부 설정
	addRes.frac = fracT; //10비트로 정규화

	return addRes.result;
}

hpfp multiply_function(hpfp a, hpfp b) {
	hpfpS mulRes; //결과값 객체
	unsigned short signA = a >> 15; //부호비트 a
	unsigned short expA = a << 1; //지수비트 a
	expA >>= 11;
	unsigned short fracA = a << 6; //가수비트 a
	fracA >>= 6;

	unsigned short signB = b >> 15;//부호비트 b
	unsigned short expB = b << 1;//지수비트 b
	expB >>= 11;
	unsigned short fracB = b << 6; //가수비트 b
	fracB >>= 6;
	unsigned short sRes = signA ^ signB;
	mulRes.sBit = sRes; //결과 부호비트 설정

	if (((expA == 31) && (fracA != 0)) || ((expB == 31) && (fracB != 0))) { //NaN이 포함되어 있을 경우
		mulRes.exp = 31; //NaN으로 설정
		mulRes.frac = 1;
		return mulRes.result;
	}

	if (((expA == 31) && (fracA == 0)) || ((expB == 31) && (fracB == 0))) { //a 또는 b가 무한대일 경우
		mulRes.exp = 31; //무한대로 설정
		mulRes.frac = 0;
		return mulRes.result;
	}
	unsigned int expT = expA + expB - 15; //지수값을 더한 후 바이어스값 빼기
	if (expT >= 31) { //오버플로우 발생한 경우 무한대로 처리
		mulRes.exp = 31;
		mulRes.frac = 0;
		return mulRes.result;
	}

	mulRes.exp = expT; //지수값 대입
	unsigned int fracT = (fracA + 0x400) * (fracB+0x400); //가수 비트 곱하기

	if (fracT >> 21 == 1) { //가수 곱셈 결과의 22번째 비트가 1일 경우

		mulRes.exp += 1; //지수값 +1

		unsigned int round = (fracT & 0x400) >> 10; //반올림 기준이 되는 11번째 비트만 구하기
		if (round == 0) { //0일 경우 반올림 안함
			mulRes.frac = (fracT >> 11);
		}
		else if (round == 1) { //1일 경우
			unsigned int half = (fracT << 22) >> 22; //오른쪽에서 1~10번째 비트 값만 남기기
			if (half == 0) { //0일 경우 (1/2일 경우)
				unsigned int zeroF = (fracT & 0x800) >> 11; //12번째 비트 구하기
				if (zeroF == 1) { //1일 경우 반올림 실행
					mulRes.frac = (fracT >> 11) + 1;
				}
				else if (zeroF == 0) { //0일 경우 반올림 실행하지 않음
					mulRes.frac = fracT >> 11;
				}
			}
			else { //half가 0이 아닐 경우
				mulRes.frac = (fracT >> 11) + 1; //반올림
			}
		}
	}
	else {
		unsigned int round = (fracT & 0x200) >> 9; //반올림 기준이 되는 11번째 비트만 구하기
		if (round == 0) { //0일 경우 반올림 안함
			mulRes.frac = (fracT >> 10);
		}
		else if (round == 1) { //1일 경우
			unsigned int half = (fracT << 23) >> 23; //오른쪽에서 1~9번째 비트 값만 남기기
			if (half == 0) { //0일 경우 (1/2일 경우)
				unsigned int zeroF = (fracT & 0x400) >> 10; //11번째 비트 구하기
				if (zeroF == 1) { //1일 경우 반올림 실행
					mulRes.frac = (fracT >> 10) + 1;
				}
				else if (zeroF == 0) { //0일 경우 반올림 실행하지 않음
					mulRes.frac = fracT >> 10;
				}
			}
			else { //half가 0이 아닐 경우
				mulRes.frac = (fracT >> 10) + 1; //반올림
			}
		}
	}
	return mulRes.result;
}

char* comparison_function(hpfp a, hpfp b) {
	char* charRes = 0;
	unsigned int signA = a >> 15; //a 부호 비트
	unsigned int expA = a << 1; //a 지수부 비트
	expA >>= 11;
	unsigned int fracA = a << 6; //a 가수부 비트
	fracA >>= 6;

	unsigned int signB = b >> 15; //b 부호 비트
	unsigned int expB = b << 1; //b 지수부 비트
	expB >>= 11;
	unsigned int fracB = b << 6; //b 가수부 비트
	fracB >>= 6;

	if ((expA == 31 && fracA == 0) && (expB == 31 && fracB == 0)) { //a,b 모두 무한대일 경우
		if (signA == signB) //부호가 같을 경우
			charRes = "=";
		else if (signA == 0) //a가 양수일 경우
			charRes = ">";
		else //b가 양수일 경우
			charRes = "<";
	}
	else if (expA == 31 && fracA != 0) { //a가 NaN일 경우
		charRes = "=";
	}
	else if (expB == 31 && fracB != 0) { //b가 NaN일 경우
		charRes = "=";
	}
	else if (expA == 31 && fracA == 0) {//a만 무한대일 경우
		if (signA == 0) //부호가 +일 경우
			charRes = ">";
		else  //부호가 -일 경우
			charRes = "<";
	}
	else if (expB == 31 && fracB == 0) { //b만 무한대일 경우
		if (signB == 0) //부호가 +일 경우
			charRes = "<";
		else if (signB == 1) //부호가 -일 경우
			charRes = ">";
	}
	else { //a,b 모두 normal value일 경우
		if ((signA == 0) && (signB == 1)) //a가 양수, b가 음수일 경우
			charRes = ">";
		else if ((signA == 1) && (signB == 0)) //a가 음수, b가 양수일 경우
			charRes = "<";
		else if (signA == 0) { //부호가 양수로 같을 경우
			if (expA > expB) //a 지수가 더 클 경우
				charRes = ">";
			else if (expA < expB) //b 지수가 더 클 경우
				charRes = "<";
			else { //a, b 지수가 같을 경우
				if (fracA > fracB) //a 가수가 더 클 경우
					charRes = ">";
				else if (fracA < fracB) //b 가수가 더 클 경우
					charRes = "<";
				else //a, b 두 숫자가 모두 같을 경우
					charRes = "=";
			}
		}
		else { //부호가 음수로 같을 경우
			if (expA > expB) //a 지수가 더 클 경우
				charRes = "<";
			else if (expA < expB) //b 지수가 더 클 경우
				charRes = ">";
			else { //a, b 지수가 같을 경우
				if (fracA > fracB) //a 가수가 더 클 경우
					charRes = "<";
				else if (fracA < fracB) //b 가수가 더 클 경우
					charRes = ">";
				else //a, b 두 숫자가 모두 같을 경우
					charRes = "=";
			}
		}
	}
	return charRes;
}

float float_flipper(float input) {
	hpfp beforeNum = float_converter(input); //input을 hpfp형으로 변환
	hpfp tempNum = ~beforeNum; //비트 not 연산
	float afterNum = hpfp_to_float_converter(tempNum); //hpfp형인 tempnum을 float형으로 변환
	return afterNum;
}

char* hpfp_to_bits_converter(hpfp result) { //2바이트 수를 비트 표현으로 char 배열로 저장

	char* bitArr = (char*)malloc(sizeof(char) * 17); //16비트 + 1 만큼 동적할당

	for (int i = 15; i >= 0; i--) {
		bitArr[i] = '0' + (result & 1); //result 끝자리가 1이면 1, 0이면 0 배열에 저장
		result >>= 1; //result 우측 시프트
	}
	bitArr[16] = '\0'; //문자열 한번에 출력 위해 '\0' 문자열 마지막 요소에 삽입

	//함수 호출 쪽에서 동적할당메모리 사용해야 하므로 메모리 해제 안함

	return bitArr;
}