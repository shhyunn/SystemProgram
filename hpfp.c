#include "hpfp.h"
#include <stdlib.h>

typedef union hpfpStruct { //����ü ��Ʈ �迭 ����
	hpfp result;
	struct {
		hpfp frac : 10; //���� 10��Ʈ
		hpfp exp : 5; //���� 5��Ʈ
		hpfp sBit : 1; //��ȣ 1��Ʈ
	};
} hpfpS;

typedef union floatStruct {
	float num;
	struct { //��Ʈ ���� ���� int������ ����
		unsigned int numInt;
	};
} floatS;

hpfp int_converter(int input) {
	hpfpS hfs1; //��ü ����

	hfs1.sBit = 0; //��ȣ ��Ʈ 0���� �ʱ�ȭ

	if (input < 0) { //�������� ������ ���
		hfs1.sBit = 1; //��ȣ��Ʈ 1�� ����
		input = -input; //������ ����� ��ȯ
	}

	if (input == 0) { //input ���� 0�� ���
		hfs1.sBit = 0; //��ȣ ��Ʈ 0���� ����
		hfs1.exp = 0; //���� ��Ʈ 0���� ����
		hfs1.frac = 0; //���� ��Ʈ 0���� ����

		return hfs1.result;
	}

	unsigned int inputCopy = input; //���纻 ���� ���
	unsigned int inputCopy2 = input; 

	int cnt = 0; //0���� �ʱ�ȭ
	for (int i = 0; i < 32; i++) { //int�� 4����Ʈ�̹Ƿ�, 32���� �ݺ�
		if (inputCopy == 0) //0�� �Ǹ� �ݺ��� ����
			break;
		inputCopy >>= 1; //�������� ��Ʈ����Ʈ
		cnt += 1; //��Ʈ �ڸ� +1
	}

	unsigned int exp = (cnt - 1) + 15; //������ ���
	if (exp >= 31) {//5��Ʈ �Ѿ ���
		hfs1.exp = 31; //���Ѵ� ���� ó��
		hfs1.frac = 0;
	}
	else {
		hfs1.exp = exp; //�״�� ����
		hfs1.frac = inputCopy2 << (10 - (cnt - 1)); // 10-���� ���� ��ŭ ���� ����Ʈ
	}
	return hfs1.result;
}

int hpfp_to_int_converter(hpfp input) {
	int intRes = 0; //��ȣ�� �ִ� int�� ���� ����
	unsigned short sign = input >> 15; //��ȣ ��Ʈ�� �����
	unsigned short exp = input << 1; //������ ��Ʈ�� �����
	exp >>= 11;
	unsigned short frac = input << 6; //������ ��Ʈ�� �����
	frac >>= 6;

	if (exp == 31) { //�����ΰ� ���Ѵ��� ���
		if (frac == 0) { //������ ��Ʈ�� 0�� ���(���Ѵ�)
			if (sign == 0) {//��ȣ ��Ʈ�� 0�� ��� TMAX
				intRes = 0; 
				for (int i = 0; i < 31; i++) { //TMAX�� ��Ʈ�迭 �����
					intRes <<= 1;
					intRes += 1; //01111....
				}
			}
			else if (sign == 1) {//��ȣ ��Ʈ�� 1�� ��� TMIN
				intRes = 1;
				for (int j = 0; j < 31; j++) { //TMIN���� ��Ʈ�迭 �����
					intRes <<= 1;
					intRes += 1; //1111111....
				}
			}
		}
		else if (frac != 0) { //������ ��Ʈ 1�� �ϳ��� ���ԵǾ� ���� ��� (NaN) TMIN
			intRes = 1;
			for (int k = 0; k < 31; k++) { //TMIN���� ��Ʈ�迭 �����
				intRes <<= 1;
				intRes += 1;
			}
		}
	}
	else if (exp == 0 && frac == 0) { //������, ������ ��� 0�� ���
		intRes = 0;
	}
	else { //�����ΰ� ���Ѵ밡 �ƴ� ���
		int expReal = exp - 15; //���� ���� ���ϱ� ���� ���̾�� 15 ����
		if (expReal >= 0) { //0 �̻��� ��� ���� ����Ʈ
			frac >>= (10 - expReal); //(10-���� ����)��ŭ ������ ��������Ʈ
			frac += (1 << expReal); //�� �տ� 1 �ٿ��ֱ�
			if (sign == 0) { //��ȣ��Ʈ�� 0�� ��� �״�� ������� ����
				intRes = frac;
			}
			else if (sign == 1) { //��ȣ��Ʈ�� 1�� ��� ������ ����� ����
				intRes = -frac;
			}
		}
		else if (expReal < 0) { //round-to-zero ����
		intRes = 0;
		}
	}
	return intRes;

}

hpfp float_converter(float input) {
	hpfpS hfs2; //��ü ����
	floatS floatNum; //float�� ��Ʈ ���� ���� ��ü ����

	floatNum.num = input; //float������ input�� ����
	hfs2.sBit = floatNum.numInt >> 31; //���� ���� ��Ʈ ��� ���Ͽ� ������Ʈ����Ʈ
	if (input == 0) { //input ���� 0�� ���
		hfs2.exp = 0;
		hfs2.frac = 0;
		return hfs2.result;
	}
	unsigned int tempExp = floatNum.numInt << 1; //������ ��� ���� ��Ʈ����Ʈ
	unsigned int tempFrac = floatNum.numInt << 9; //��ȣ��Ʈ�� ������ ��Ʈ ����
	tempFrac >>= 22; //10��Ʈ �����

	tempExp >>= 24; //8��Ʈ ����� ���� ���� ��Ʈ����Ʈ
	tempExp = tempExp - 127; //���� ���� ��� ���� 32��Ʈ ���̾ �� 127 ����

	unsigned int exp = tempExp + 15; //���̾�� 15 ���Ͽ� ������ ����
	if (exp >= 31 && tempFrac == 0) {//31�̻��� ��� ���Ѵ� ó��
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
	floatS result; //float�� ���� ����
	unsigned short sign = input >> 15; //��ȣ��Ʈ
	unsigned short exp = input << 1; //������ ��Ʈ
	exp >>= 11;

	unsigned short frac = input << 6; //������ ��Ʈ
	frac >>= 6;

	result.numInt = sign; //�ʱⰪ ��ȣ ��Ʈ�� ����
	if (exp == 31) { //������ ��Ʈ�� ��� 1�� ���
		for (int i = 0; i < 8; i++) { //8��Ʈ���� �ݺ�
			result.numInt <<= 1; //���� ����Ʈ
			result.numInt += 1; //8��Ʈ ��� 1�� ����
		}
		if (frac == 0) { //������ ��Ʈ�� 0�� ���
			result.numInt <<= 23; //��� 0���� ä���
		}
		else if (frac != 0) { //������ ��Ʈ�� 0�� �ƴ� ���
			result.numInt <<= 23;
			result.numInt += 1; //������ ��Ʈ 0�� �ƴ� ������ ä���,NaN
		}
	}
	else if (exp == 0 && frac == 0) { //������, ������ ��Ʈ ��� 0�� ���
		result.num = 0;
	}
	else { //������ ��Ʈ�� ��� 1�� �ƴ� ���
		int expReal = exp - 15; //���̾�� 15 ���� ���� ������ ���ϱ�
		exp = expReal + 127; //�ٽ� ���̾�� 127 ���ؼ� exp�� ����
		for (int k = 7; k >= 0; --k) { //������ ��Ʈ 8��Ʈ ���� �ݺ�
			int temp = exp >> k & 1;
			result.numInt <<= 1;
			result.numInt += temp;
		}
		for (int l = 9; l >= 0; --l) { //������ ��Ʈ 10��Ʈ ���� �ݺ�
			int temp2 = frac >> l & 1;
			result.numInt <<= 1;
			result.numInt += temp2;
		}
		result.numInt <<= 13; //������ ������ ��Ʈ 23��Ʈ ä��� ���� 13��ŭ ���� ��Ʈ����Ʈ
	}
	return result.num; //float������ ��ȯ
}

hpfp addition_function(hpfp a, hpfp b) {
	hpfpS addRes; //����� ��ü
	unsigned int add = 1; //add �÷��� 1�� �ʱ�ȭ
	unsigned int order = 1; //���� �÷��� 1�� �ʱ�ȭ
	unsigned short signA = a >> 15; //��ȣ��Ʈ a
	unsigned short expA = a << 1; //������Ʈ a
	expA >>= 11;
	unsigned short fracA = a << 6; //������Ʈ a
	fracA >>= 6;

	unsigned short signB = b >> 15;//��ȣ��Ʈ b
	unsigned short expB = b << 1;//������Ʈ b
	expB >>= 11;
	unsigned short fracB = b << 6; //������Ʈ b
	fracB >>= 6;

	addRes.sBit = signA; //��� ��ȣ��Ʈ a ��ȣ��Ʈ�� ����

	if (((expA == 31) && (fracA != 0)) || ((expB == 31) && (fracB != 0))) { //NaN�� ���ԵǾ� ���� ���
		addRes.exp = 31; //NaN���� ����
		addRes.frac = 1;
		return addRes.result;
	}

	if (((expA == 31) && (fracA == 0)) && ((expB == 31) && (fracB == 0))) { //a,b ��� ���Ѵ��� ���
		addRes.exp = 31;
		addRes.frac = 0;
		if (signA != signB) { //a,b�� ��ȣ�� �ٸ� ���
			addRes.frac = 1; //NaN���� ó��
		}
		return addRes.result;
	}
	else if ((expA == 31) && (fracA == 0)) { //a�� ���Ѵ��� ���
		addRes.exp = 31;
		addRes.frac = 0;
		return addRes.result; //���Ѵ�� ó��
	}
	else if ((expB == 31) && (fracB == 0)) { //b�� ���Ѵ��� ���
		addRes.sBit = signB; //��ȣ��Ʈ b�� ����
		addRes.exp = 31;
		addRes.frac = 0;
		return addRes.result; //���Ѵ�� ó��
	}

	if (signA != signB) { //��ȣ�� �ٸ� ���
		add = 0; //������ ���� ����
		if (expA > expB) { //a ������ �� Ŭ ���
			addRes.sBit = signA; //a ��ȣ��Ʈ�� ����
		}
		else { //b ������ �� Ŭ ���
			addRes.sBit = signB; //b ��ȣ��Ʈ�� ����
			order = 0; //b���� a ������ ���� ����
		}
	}

	int expDiff = expA - expB; //���� �� ���ϱ�
	fracA += 0x400;
	fracB += 0x400;

	if (expDiff > 0) { //a ������ �� Ŭ ���
		fracB >>= expDiff; //b ������ ���� ����Ʈ
		expB = expA; //a ������ �ٲٱ�
	}
	else if (expDiff < 0) { //b ������ �� Ŭ ���
		fracA >>= -expDiff ; //a ������ ���� ����Ʈ
		expA = expB; //b ������ �ٲٱ�
	}

	unsigned short fracT; //������ ����� ����

	if (add == 1) { //���� ���
		fracT = fracA + fracB;
	}
	else { //�� ���
		if (order == 1) { //a->b ������ ���� ������ ���
			fracT = fracA - fracB;
		}
		else { //b->a ������ ���� ������ ���
			fracT = fracB - fracA;
		}
	}

	if (((fracT & 0x800) >> 11) == 1) { //������ 11��Ʈ �ʰ�
		if ((fracT & 1) == 1) { //Ȧ���� ���
			fracT >>= 1; 
			expA += 1; 
			fracT += 1; 
		}
		else { //¦���� ���
			expA += 1; //�ݿø�
			fracT >>= 1;
		}
	}

	addRes.exp = expA; //������ ����
	addRes.frac = fracT; //10��Ʈ�� ����ȭ

	return addRes.result;
}

hpfp multiply_function(hpfp a, hpfp b) {
	hpfpS mulRes; //����� ��ü
	unsigned short signA = a >> 15; //��ȣ��Ʈ a
	unsigned short expA = a << 1; //������Ʈ a
	expA >>= 11;
	unsigned short fracA = a << 6; //������Ʈ a
	fracA >>= 6;

	unsigned short signB = b >> 15;//��ȣ��Ʈ b
	unsigned short expB = b << 1;//������Ʈ b
	expB >>= 11;
	unsigned short fracB = b << 6; //������Ʈ b
	fracB >>= 6;
	unsigned short sRes = signA ^ signB;
	mulRes.sBit = sRes; //��� ��ȣ��Ʈ ����

	if (((expA == 31) && (fracA != 0)) || ((expB == 31) && (fracB != 0))) { //NaN�� ���ԵǾ� ���� ���
		mulRes.exp = 31; //NaN���� ����
		mulRes.frac = 1;
		return mulRes.result;
	}

	if (((expA == 31) && (fracA == 0)) || ((expB == 31) && (fracB == 0))) { //a �Ǵ� b�� ���Ѵ��� ���
		mulRes.exp = 31; //���Ѵ�� ����
		mulRes.frac = 0;
		return mulRes.result;
	}
	unsigned int expT = expA + expB - 15; //�������� ���� �� ���̾�� ����
	if (expT >= 31) { //�����÷ο� �߻��� ��� ���Ѵ�� ó��
		mulRes.exp = 31;
		mulRes.frac = 0;
		return mulRes.result;
	}

	mulRes.exp = expT; //������ ����
	unsigned int fracT = (fracA + 0x400) * (fracB+0x400); //���� ��Ʈ ���ϱ�

	if (fracT >> 21 == 1) { //���� ���� ����� 22��° ��Ʈ�� 1�� ���

		mulRes.exp += 1; //������ +1

		unsigned int round = (fracT & 0x400) >> 10; //�ݿø� ������ �Ǵ� 11��° ��Ʈ�� ���ϱ�
		if (round == 0) { //0�� ��� �ݿø� ����
			mulRes.frac = (fracT >> 11);
		}
		else if (round == 1) { //1�� ���
			unsigned int half = (fracT << 22) >> 22; //�����ʿ��� 1~10��° ��Ʈ ���� �����
			if (half == 0) { //0�� ��� (1/2�� ���)
				unsigned int zeroF = (fracT & 0x800) >> 11; //12��° ��Ʈ ���ϱ�
				if (zeroF == 1) { //1�� ��� �ݿø� ����
					mulRes.frac = (fracT >> 11) + 1;
				}
				else if (zeroF == 0) { //0�� ��� �ݿø� �������� ����
					mulRes.frac = fracT >> 11;
				}
			}
			else { //half�� 0�� �ƴ� ���
				mulRes.frac = (fracT >> 11) + 1; //�ݿø�
			}
		}
	}
	else {
		unsigned int round = (fracT & 0x200) >> 9; //�ݿø� ������ �Ǵ� 11��° ��Ʈ�� ���ϱ�
		if (round == 0) { //0�� ��� �ݿø� ����
			mulRes.frac = (fracT >> 10);
		}
		else if (round == 1) { //1�� ���
			unsigned int half = (fracT << 23) >> 23; //�����ʿ��� 1~9��° ��Ʈ ���� �����
			if (half == 0) { //0�� ��� (1/2�� ���)
				unsigned int zeroF = (fracT & 0x400) >> 10; //11��° ��Ʈ ���ϱ�
				if (zeroF == 1) { //1�� ��� �ݿø� ����
					mulRes.frac = (fracT >> 10) + 1;
				}
				else if (zeroF == 0) { //0�� ��� �ݿø� �������� ����
					mulRes.frac = fracT >> 10;
				}
			}
			else { //half�� 0�� �ƴ� ���
				mulRes.frac = (fracT >> 10) + 1; //�ݿø�
			}
		}
	}
	return mulRes.result;
}

char* comparison_function(hpfp a, hpfp b) {
	char* charRes = 0;
	unsigned int signA = a >> 15; //a ��ȣ ��Ʈ
	unsigned int expA = a << 1; //a ������ ��Ʈ
	expA >>= 11;
	unsigned int fracA = a << 6; //a ������ ��Ʈ
	fracA >>= 6;

	unsigned int signB = b >> 15; //b ��ȣ ��Ʈ
	unsigned int expB = b << 1; //b ������ ��Ʈ
	expB >>= 11;
	unsigned int fracB = b << 6; //b ������ ��Ʈ
	fracB >>= 6;

	if ((expA == 31 && fracA == 0) && (expB == 31 && fracB == 0)) { //a,b ��� ���Ѵ��� ���
		if (signA == signB) //��ȣ�� ���� ���
			charRes = "=";
		else if (signA == 0) //a�� ����� ���
			charRes = ">";
		else //b�� ����� ���
			charRes = "<";
	}
	else if (expA == 31 && fracA != 0) { //a�� NaN�� ���
		charRes = "=";
	}
	else if (expB == 31 && fracB != 0) { //b�� NaN�� ���
		charRes = "=";
	}
	else if (expA == 31 && fracA == 0) {//a�� ���Ѵ��� ���
		if (signA == 0) //��ȣ�� +�� ���
			charRes = ">";
		else  //��ȣ�� -�� ���
			charRes = "<";
	}
	else if (expB == 31 && fracB == 0) { //b�� ���Ѵ��� ���
		if (signB == 0) //��ȣ�� +�� ���
			charRes = "<";
		else if (signB == 1) //��ȣ�� -�� ���
			charRes = ">";
	}
	else { //a,b ��� normal value�� ���
		if ((signA == 0) && (signB == 1)) //a�� ���, b�� ������ ���
			charRes = ">";
		else if ((signA == 1) && (signB == 0)) //a�� ����, b�� ����� ���
			charRes = "<";
		else if (signA == 0) { //��ȣ�� ����� ���� ���
			if (expA > expB) //a ������ �� Ŭ ���
				charRes = ">";
			else if (expA < expB) //b ������ �� Ŭ ���
				charRes = "<";
			else { //a, b ������ ���� ���
				if (fracA > fracB) //a ������ �� Ŭ ���
					charRes = ">";
				else if (fracA < fracB) //b ������ �� Ŭ ���
					charRes = "<";
				else //a, b �� ���ڰ� ��� ���� ���
					charRes = "=";
			}
		}
		else { //��ȣ�� ������ ���� ���
			if (expA > expB) //a ������ �� Ŭ ���
				charRes = "<";
			else if (expA < expB) //b ������ �� Ŭ ���
				charRes = ">";
			else { //a, b ������ ���� ���
				if (fracA > fracB) //a ������ �� Ŭ ���
					charRes = "<";
				else if (fracA < fracB) //b ������ �� Ŭ ���
					charRes = ">";
				else //a, b �� ���ڰ� ��� ���� ���
					charRes = "=";
			}
		}
	}
	return charRes;
}

float float_flipper(float input) {
	hpfp beforeNum = float_converter(input); //input�� hpfp������ ��ȯ
	hpfp tempNum = ~beforeNum; //��Ʈ not ����
	float afterNum = hpfp_to_float_converter(tempNum); //hpfp���� tempnum�� float������ ��ȯ
	return afterNum;
}

char* hpfp_to_bits_converter(hpfp result) { //2����Ʈ ���� ��Ʈ ǥ������ char �迭�� ����

	char* bitArr = (char*)malloc(sizeof(char) * 17); //16��Ʈ + 1 ��ŭ �����Ҵ�

	for (int i = 15; i >= 0; i--) {
		bitArr[i] = '0' + (result & 1); //result ���ڸ��� 1�̸� 1, 0�̸� 0 �迭�� ����
		result >>= 1; //result ���� ����Ʈ
	}
	bitArr[16] = '\0'; //���ڿ� �ѹ��� ��� ���� '\0' ���ڿ� ������ ��ҿ� ����

	//�Լ� ȣ�� �ʿ��� �����Ҵ�޸� ����ؾ� �ϹǷ� �޸� ���� ����

	return bitArr;
}