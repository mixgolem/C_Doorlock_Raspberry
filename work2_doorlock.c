//complete
#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <softTone.h>
#include <string.h>
#include <stdlib.h>

#define PW_LEN 4
#define FAIL_CNT 3

#define B1 1
#define B2 2
#define B3 4
#define B4 8
#define B5 16
#define B6 32
#define B7 64
#define B8 128
#define B9 256
#define BS 512
#define B0 1024
#define BH 2048

#define KEYPAD_PB1 23
#define KEYPAD_PB2 24
#define KEYPAD_PB3 25
#define KEYPAD_PB5 8
#define KEYPAD_PB6 7
#define KEYPAD_PB7 12
#define KEYPAD_PB9 16
#define KEYPAD_PB10 20
#define KEYPAD_PB11 21
#define KEYPAD_PB13 13
#define KEYPAD_PB14 19
#define KEYPAD_PB15 26
#define MAX_KEY_BT_NUM 12

#define SERVO 18
#define BUZZER_PIN 17
#define DO_L 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define RA 880
#define SI 987
#define DO_H 1046

const int KeypadTable[MAX_KEY_BT_NUM] = {
    KEYPAD_PB1, KEYPAD_PB2, KEYPAD_PB3, KEYPAD_PB5, KEYPAD_PB6, KEYPAD_PB7,
    KEYPAD_PB9, KEYPAD_PB10, KEYPAD_PB11, KEYPAD_PB13, KEYPAD_PB14, KEYPAD_PB15
};

// 키패드의 상태를 읽어오는 함수
int KeypadRead(void) {
    int nKeypadstate = 0;

    for (int i = 0; i < MAX_KEY_BT_NUM; i++) {
        if (!digitalRead(KeypadTable[i])) {
            nKeypadstate |= (1 << i);
        }
    }

    return nKeypadstate;
}

// 각 음계에 해당하는 주파수 값을 선택하는 함수
unsigned int SevenScale(unsigned char scale) {
    unsigned int _ret = 0;

    switch (scale) {
    case 0:
        _ret = DO_L;
        break;
    case 1:
        _ret = RE;
        break;
    case 2:
        _ret = MI;
        break;
    case 3:
        _ret = FA;
        break;
    case 4:
        _ret = SOL;
        break;
    case 5:
        _ret = RA;
        break;
    case 6:
        _ret = SI;
        break;
    case 7:
        _ret = DO_H;
        break;
    }

    return _ret;
}

void Change_FREQ(unsigned int freq) {
    softToneWrite(BUZZER_PIN, freq);
}

void STOP_FREQ(void) {
    softToneWrite(BUZZER_PIN, 0);
}

void Buzzer_Init(void) {
    softToneCreate(BUZZER_PIN);
    STOP_FREQ();
}

void door_open() {
    printf("Door Open\n");
    softPwmWrite(SERVO, 25);
    for (int i = 0; i < 3; i++) {
        Change_FREQ(SevenScale(i));
        delay(200);
        STOP_FREQ();
    }
    delay(2000);
    softPwmWrite(SERVO, 5);
    for (int i = 2; i >= 0; i--) {
        Change_FREQ(SevenScale(i));
        delay(200);
        STOP_FREQ();
    }
}

void pw_fail(int fail) {
    // 비밀번호가 틀렸을 경우 소리와 함께 화면에 메세지 출력
    printf("\nIncorrect Password\n ");
    for (int i = 0; i < fail; i++) {
        Change_FREQ(SevenScale(7));
        delay(200);
        STOP_FREQ();
        delay(200);
    }
    if (fail >= FAIL_CNT) { // 실패 카운트만큼 틀린 경우 10초 대기
        printf("\n(Input is limited for 10 seconds)\n");
        delay(10000);
    }
}

void printint(int *arr) {
    printf("\npw print\n");
    for (int i = 0; i < PW_LEN; i++) {
        printf("\n%d\n ", arr[i]);
    }
}

int pwcmp(int *a, int *b) {
    if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3]) return 1;
    else return 0;
}

int main() {
    if (wiringPiSetupGpio() == -1)
        return 1;
    softPwmCreate(SERVO, 0, 200);
    Buzzer_Init();

    int past_key = 0;
    int past_num = 0;
    int *pw = calloc(PW_LEN, sizeof(int));
    int *buf = calloc(PW_LEN, sizeof(int));
    pw[0] = 1;
    pw[1] = 2;
    pw[2] = 4;
    pw[3] = 8;
    
    // 키패드 핀을 입력으로 설정
    for (int i = 0; i < MAX_KEY_BT_NUM; i++) {
        pinMode(KeypadTable[i], INPUT);
    }

    int t = 0;  // 입력에 대한 카운트
    int fail = 0; // 비밀번호 오답에 대한 카운트
    printint(pw);
    while (1) {
        int key = KeypadRead(); // 키를 계속 인식
        
        if (key != 0 && past_key == 0) { // 키가 눌렸다 떼지는 순간 동작
            Change_FREQ(SevenScale(2));
            delay(200);
            STOP_FREQ();
            if (key == BH) { //샵버튼일 경우
                //delay(200);
                memset(pw, 0, PW_LEN * sizeof(int)); // 비밀번호 0으로 초기화
                printf("\nInput new password >> \n");
                while (1) { // 새로운 비밀번호를 입력받을 반복문
                    int num = KeypadRead();

                    if (num == BH && past_num == 0 && t == PW_LEN) { // 비밀번호 4자리 입력이 완료된 경우
                        Change_FREQ(SevenScale(2));
                        delay(200);
                        STOP_FREQ();
                        printf("\nPassword setup complete\n");
                        printint(pw); // 입력한 비밀번호를 출력해줌
                        
                        t = 0;
                        break;
                    }
                    if (num != 0 && past_num == 0 && num != BS && num != BH && t < PW_LEN) { // 숫자버튼이 인식되는 순간
                        Change_FREQ(SevenScale(2));
                        delay(200);
                        STOP_FREQ();
                        
                        pw[t] = num;
                        
                        printf("\nt:%d num:(%d) pw:[%d]\n",t, num, pw[t]); // 디버그용
                        t++;
                    }
                    past_num = num;
                }
            } else if (key != BS && t < PW_LEN) { // 숫자버튼일경우
                //delay(200);
                buf[t] = key;
                printf("\nt:%d key:(%d) b:[%d]\n",t, key, buf[t]); // 디버그용
                
                t++; // 숫자버튼을 버퍼에 입력
            } else if (key == BS && t == PW_LEN) { // 4자리 입력하고 별 누를 경우
                //delay(200);
                t = 0;
                if (pwcmp(pw, buf)==1){
                    fail = 0;
                    door_open(); // 비밀번호 맞으면 문 열림
                }

                else {
                    fail++; // 틀린 경우 fail카운터 증가
                    pw_fail(fail);
                    printint(pw);
                }
            }
        }
        past_key = key;
    }

    free(buf);
    free(pw);
    return 0;
}
