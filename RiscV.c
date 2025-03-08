#define _CRT_SECURE_NO_WARNINGS  // ���� ��� ��Ȱ��ȭ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>



#define MAX_LINE_LENGTH 256
#define MAX_LABELS 1000
#define MAX_INSTRUCTIONS 5000
#define NUM_REGISTERS 32

typedef struct {
    char name[MAX_LINE_LENGTH];
    uint32_t address;
} Label;

typedef struct {
    uint32_t address;
    char instruction[MAX_LINE_LENGTH];
    uint32_t machine_code;
} Instruction;

// ���� ����: ���̺�� ��ɾ�
Label labels[MAX_LABELS];
int label_count = 0;

Instruction instructions[MAX_INSTRUCTIONS];
int instruction_count = 0;

// �������� �ʱ�ȭ
uint32_t registers_arr[NUM_REGISTERS] = { 0 };

// �Լ� ������Ÿ��
void trim_whitespace(char* str);
int is_label_definition(char* line, char* label);
int add_label(char* label, uint32_t address);
int get_label_address(char* label);
int parse_register(char* token);
int encode_instruction(char* line, uint32_t current_address);
int first_pass(FILE* fp);
int second_pass();
int simulate_execution(char* trace_filename);
void to_uppercase(char* str);
int write_machine_code(char* o_filename);
void uint32_to_binary(uint32_t n, char* binary_str);

// 'registers' �̸� ����
#define registers registers_arr

#ifdef _WIN32
    #define STRICMP _stricmp
#else
    #define STRICMP strcasecmp
#endif

int main() {
    char filename[MAX_LINE_LENGTH];

    // Ư�� �������� �ʱ�ȭ
    registers[1] = 1;
    registers[2] = 2;
    registers[3] = 3;
    registers[4] = 4;
    registers[5] = 5;
    registers[6] = 6;

    while (1) {
        printf("Enter Input File Name: ");
        if (scanf("%255s", filename) != 1) {  // C6031: ��ȯ �� Ȯ��
            fprintf(stderr, "�Է� �б� ����.\n");
            // �Է� ���� ����
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        if (strcmp(filename, "terminate") == 0) {
            break;
        }

        FILE* fp = fopen(filename, "r");
        if (fp == NULL) {
            printf("Input file does not exist!!\n");
            continue;
        }

        // ���̺�� ��ɾ� �ʱ�ȭ
        label_count = 0;
        instruction_count = 0;

        // 1�� �н�: ���̺� ����
        if (first_pass(fp) != 0) {
            printf("Syntax Error!!\n");
            fclose(fp);
            continue;
        }

        // 2�� �н��� ���� ���� ������ �缳��
        fseek(fp, 0, SEEK_SET);

        // 2�� �н�: ��ɾ� ���ڵ�
        if (second_pass() != 0) {  // FILE *fp �Ű����� ����
            printf("Syntax Error!!\n");
            fclose(fp);
            continue;
        }

        fclose(fp);

        // .o �� .trace ���� �̸� ����
        char o_filename[MAX_LINE_LENGTH];

        strncpy(o_filename, filename, MAX_LINE_LENGTH -1);
        o_filename[MAX_LINE_LENGTH - 1] = '\0';

        char* dot = strrchr(o_filename, '.');
        if (dot) *dot = '\0';

        strncat(o_filename, ".o", MAX_LINE_LENGTH - strlen(o_filename) - 1);
        char trace_filename[MAX_LINE_LENGTH];

        strncpy(trace_filename, filename, MAX_LINE_LENGTH - 1);
        trace_filename[MAX_LINE_LENGTH - 1] = '\0';
        dot = strrchr(trace_filename, '.');
        if (dot) *dot = '\0';

        strncat(trace_filename, ".trace", MAX_LINE_LENGTH - strlen(trace_filename) - 1);

        // .o ���Ͽ� ���� ����
        if (write_machine_code(o_filename) != 0) {
            printf("Error writing to .o file!\n");
            continue;
        }

        // �ùķ��̼� ���� �� .trace ���� ����
        if (simulate_execution(trace_filename) != 0) {
            printf("Error during simulation!\n");
            continue;
        }

        printf("Files '%s' and '%s' generated successfully.\n", o_filename, trace_filename);
    }

    return 0;
}

// ���ڿ��� �յ� ���� ����
void trim_whitespace(char* str) {
    // ���� ���� ����
    int index = 0;
    while (isspace((unsigned char)str[index])) index++;
    if (index != 0) memmove(str, str + index, strlen(str) - index + 1);

    // ���� ���� ����
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

// ������ ���̺� �������� Ȯ���ϰ� ���̺� �̸� ��ȯ
int is_label_definition(char* line, char* label) {
    char* colon = strchr(line, ':');
    if (colon) {
        int len = colon - line;
        if (len >= MAX_LINE_LENGTH) len = MAX_LINE_LENGTH - 1;
        // strncpy(label, MAX_LINE_LENGTH, line, len); // �߸��� �ڵ�
        // ������ �ڵ�
        strncpy(label, line, len);
        label[len] = '\0';
        trim_whitespace(label);
        return 1;
    }
    return 0;
}

// ���̺��� ���̺� ���̺� �߰�
int add_label(char* label, uint32_t address) {
    if (label_count >= MAX_LABELS) return -1;
    // strcpy(labels[label_count].name, MAX_LINE_LENGTH, label); // �߸��� �ڵ�
    // ������ �ڵ�
    strncpy(labels[label_count].name, label, MAX_LINE_LENGTH - 1);
    labels[label_count].name[MAX_LINE_LENGTH - 1] = '\0';
    labels[label_count].address = address;
    label_count++;
    return 0;
}

// ���̺��� �ּҸ� ������ (��ҹ��� ���� ����)
int get_label_address(char* label) {
    for (int i = 0; i < label_count; i++) {
        if (strcasecmp(labels[i].name, label) == 0) {  // ��ҹ��� ���� ���� ��
            return labels[i].address;
        }
    }
    return -1; // ���̺��� ã�� ����
}

// "x1" ���� �������� ��ū�� ������ ��ȯ
int parse_register(char* token) {
    if (token[0] != 'x' && token[0] != 'X') return -1;
    int reg = atoi(token + 1);
    if (reg < 0 || reg >= NUM_REGISTERS) return -1;
    return reg;
}

// ���ڿ��� �빮�ڷ� ��ȯ
void to_uppercase(char* str) {
    for (; *str; ++str) *str = toupper((unsigned char)*str);
}

// 1�� �н�: ���̺� ����
int first_pass(FILE* fp) {
    char line[MAX_LINE_LENGTH];
    uint32_t current_address = 1000;  // PC�� 1000���� �ʱ�ȭ

    while (fgets(line, sizeof(line), fp)) {
        trim_whitespace(line);
        if (strlen(line) == 0) continue; // �� �� ����

        char label[MAX_LINE_LENGTH];
        if (is_label_definition(line, label)) {
            if (add_label(label, current_address) != 0) {
                fprintf(stderr, "���̺� �߰� ����: ���̺� ���� �ʹ� �����ϴ�.\n");
                return -1;
            }
            // ���̺� ������ ��ɾ� ó��
            char* instruction = strchr(line, ':') + 1;
            trim_whitespace(instruction);
            if (strlen(instruction) > 0) {
                // ���̺� ������ ��ɾ ���� ���
                if (instruction_count >= MAX_INSTRUCTIONS) {
                    fprintf(stderr, "��ɾ� ���� �ʹ� �����ϴ�.\n");
                    return -1;
                }
                strncpy(instructions[instruction_count].instruction, instruction, MAX_LINE_LENGTH - 1);
                instructions[instruction_count].instruction[MAX_LINE_LENGTH - 1] = '\0';
                instructions[instruction_count].address = current_address;
                instruction_count++;
                current_address += 4;
            }
        }
        else {
            // �Ϲ� ��ɾ�
            if (instruction_count >= MAX_INSTRUCTIONS) {
                fprintf(stderr, "��ɾ� ���� �ʹ� �����ϴ�.\n");
                return -1;
            }
            strncpy(instructions[instruction_count].instruction, line, MAX_LINE_LENGTH - 1);
            instructions[instruction_count].instruction[MAX_LINE_LENGTH - 1] = '\0';
            instructions[instruction_count].address = current_address;
            instruction_count++;
            current_address += 4;
        }
    }
    return 0;
}

// 2�� �н�: ��ɾ� ���ڵ�
int second_pass() {
    for (int i = 0; i < instruction_count; i++) {
        uint32_t code = encode_instruction(instructions[i].instruction, instructions[i].address);
        instructions[i].machine_code = code;
    }
    // ���� ���� Ȯ�� (0xFFFFFFFE�� ���� ����)
    for (int i = 0; i < instruction_count; i++) {
        if (instructions[i].machine_code == 0xFFFFFFFE) {
            fprintf(stderr, "���� ����: ��ɾ� ���ڵ� ���� - '%s'\n", instructions[i].instruction);
            return -1;
        }
    }
    return 0;
}



// ���� ��ɾ� ������ ����� ���ڵ�
int encode_instruction(char* line, uint32_t current_address) {
    char original_line[MAX_LINE_LENGTH];
    strncpy(original_line, line, MAX_LINE_LENGTH - 1);
    original_line[MAX_LINE_LENGTH - 1] = '\0';

    // EXIT ó��
    char upper_line[MAX_LINE_LENGTH];
    strncpy(upper_line, original_line, MAX_LINE_LENGTH - 1);
    upper_line[MAX_LINE_LENGTH - 1] = '\0'; // �� ���� ����
    to_uppercase(upper_line);
    if (strcmp(upper_line, "EXIT") == 0) {
        return 0xFFFFFFFF;
    }

    // ��ɾ� ��ūȭ
    char* tokens[5];
    int token_count = 0;
    char* token = strtok(original_line, " ,\t\n");
    while (token != NULL && token_count < 5) {
        tokens[token_count++] = token;
        token = strtok(NULL, " ,\t\n");
    }

    if (token_count == 0) return 0xFFFFFFFE; // ���� ����

    // ��ɾ �빮�ڷ� ��ȯ
    to_uppercase(tokens[0]);

    // R-type ��ɾ�: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA
    if (strcmp(tokens[0], "ADD") == 0 || strcmp(tokens[0], "SUB") == 0 ||
        strcmp(tokens[0], "AND") == 0 || strcmp(tokens[0], "OR") == 0 ||
        strcmp(tokens[0], "XOR") == 0 || strcmp(tokens[0], "SLL") == 0 ||
        strcmp(tokens[0], "SRL") == 0 || strcmp(tokens[0], "SRA") == 0) {
        if (token_count != 4) return 0xFFFFFFFE; // ���� ����

        int rd = parse_register(tokens[1]);
        int rs1 = parse_register(tokens[2]);
        int rs2 = parse_register(tokens[3]);

        if (rd == -1 || rs1 == -1 || rs2 == -1) return 0xFFFFFFFE;

        uint32_t funct3, funct7;
        if (strcmp(tokens[0], "ADD") == 0) { funct3 = 0x0; funct7 = 0x00; }
        else if (strcmp(tokens[0], "SUB") == 0) { funct3 = 0x0; funct7 = 0x20; }
        else if (strcmp(tokens[0], "AND") == 0) { funct3 = 0x7; funct7 = 0x00; }
        else if (strcmp(tokens[0], "OR") == 0) { funct3 = 0x6; funct7 = 0x00; }
        else if (strcmp(tokens[0], "XOR") == 0) { funct3 = 0x4; funct7 = 0x00; }
        else if (strcmp(tokens[0], "SLL") == 0) { funct3 = 0x1; funct7 = 0x00; }
        else if (strcmp(tokens[0], "SRL") == 0) { funct3 = 0x5; funct7 = 0x00; }
        else if (strcmp(tokens[0], "SRA") == 0) { funct3 = 0x5; funct7 = 0x20; }

        uint32_t machine_code = (funct7 << 25) | (rs2 << 20) | (rs1 << 15) |
            (funct3 << 12) | (rd << 7) | 0x33;
        return machine_code;
    }

    // I-type ��ɾ�: ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, LW, JALR
    else if (strcmp(tokens[0], "ADDI") == 0 || strcmp(tokens[0], "ANDI") == 0 ||
        strcmp(tokens[0], "ORI") == 0 || strcmp(tokens[0], "XORI") == 0 ||
        strcmp(tokens[0], "SLLI") == 0 || strcmp(tokens[0], "SRLI") == 0 ||
        strcmp(tokens[0], "SRAI") == 0 || strcmp(tokens[0], "LW") == 0 ||
        strcmp(tokens[0], "JALR") == 0) {

        // LW �� JALR ó��: rd, imm(rs1)
        if (strcmp(tokens[0], "LW") == 0 || strcmp(tokens[0], "JALR") == 0) {
            if (token_count != 3) return 0xFFFFFFFE; // ���� ����
            int rd = parse_register(tokens[1]);
            if (rd == -1) return 0xFFFFFFFE;

            // offset(base) ���� �Ľ�
            char* offset_base = tokens[2];
            char* open_paren = strchr(offset_base, '(');
            char* close_paren = strchr(offset_base, ')');
            if (!open_paren || !close_paren) return 0xFFFFFFFE;
            *open_paren = '\0';
            char* offset_str = offset_base;
            char* base_str = open_paren + 1;
            int rs1 = parse_register(base_str);
            if (rs1 == -1) return 0xFFFFFFFE;
            int imm = atoi(offset_str);

            if (strcmp(tokens[0], "LW") == 0) {
                uint32_t machine_code = ((imm & 0xFFF) << 20) | (rs1 << 15) |
                    (0x2 << 12) | (rd << 7) | 0x03;
                return machine_code;
            }
            else if (strcmp(tokens[0], "JALR") == 0) {
                uint32_t funct3 = 0x0; // JALR�� funct3�� 0x0
                uint32_t machine_code = ((imm & 0xFFF) << 20) | (rs1 << 15) |
                    (funct3 << 12) | (rd << 7) | 0x67;
                return machine_code;
            }
        }
        else {
            // �Ϲ� I-type ��ɾ� ó��: rd, rs1, imm
            if (token_count != 4) return 0xFFFFFFFE; // ���� ����

            int rd = parse_register(tokens[1]);
            int rs1 = parse_register(tokens[2]);
            int imm = atoi(tokens[3]);

            if (rd == -1 || rs1 == -1) return 0xFFFFFFFE;

            uint32_t funct3, funct7 = 0x00;
            if (strcmp(tokens[0], "ADDI") == 0) { funct3 = 0x0; }
            else if (strcmp(tokens[0], "ANDI") == 0) { funct3 = 0x7; }
            else if (strcmp(tokens[0], "ORI") == 0) { funct3 = 0x6; }
            else if (strcmp(tokens[0], "XORI") == 0) { funct3 = 0x4; }
            else if (strcmp(tokens[0], "SLLI") == 0) { funct3 = 0x1; funct7 = 0x00; }
            else if (strcmp(tokens[0], "SRLI") == 0) { funct3 = 0x5; funct7 = 0x00; }
            else if (strcmp(tokens[0], "SRAI") == 0) { funct3 = 0x5; funct7 = 0x20; }

            uint32_t machine_code = 0;
            if (strcmp(tokens[0], "SLLI") == 0 || strcmp(tokens[0], "SRLI") == 0 ||
                strcmp(tokens[0], "SRAI") == 0) {
                machine_code = ((funct7 & 0x7F) << 25) | (rs1 << 15) |
                    (funct3 << 12) | (rd << 7) | 0x13;
                // SLLI, SRLI, SRAI�� shamt�� imm�� ���� 5��Ʈ�� ���
                machine_code |= (imm & 0x1F) << 20;
            }
            else {
                machine_code = ((imm & 0xFFF) << 20) | (rs1 << 15) |
                    (funct3 << 12) | (rd << 7) | 0x13;
            }
            return machine_code;
        }
    }

    // S-type ��ɾ�: SW
    else if (strcmp(tokens[0], "SW") == 0) {
        if (token_count != 3) return 0xFFFFFFFE;
        int rs2 = parse_register(tokens[1]);
        // offset(base) ���� �Ľ�
        char* offset_base = tokens[2];
        char* open_paren = strchr(offset_base, '(');
        char* close_paren = strchr(offset_base, ')');
        if (!open_paren || !close_paren) return 0xFFFFFFFE;
        *open_paren = '\0';
        char* offset_str = offset_base;
        char* base_str = open_paren + 1;
        int rs1 = parse_register(base_str);
        int imm = atoi(offset_str);
        if (rs2 == -1 || rs1 == -1) return 0xFFFFFFFE;
        uint32_t imm_upper = (imm & 0xFE0) >> 5;
        uint32_t imm_lower = imm & 0x1F;
        uint32_t machine_code = (imm_upper << 25) | (rs2 << 20) |
            (rs1 << 15) | (0x2 << 12) |
            (imm_lower << 7) | 0x23;
        return machine_code;
    }

    // B-type ��ɾ�: BEQ, BNE, BGE, BLT
    else if (strcmp(tokens[0], "BEQ") == 0 || strcmp(tokens[0], "BNE") == 0 ||
        strcmp(tokens[0], "BGE") == 0 || strcmp(tokens[0], "BLT") == 0) {
        if (token_count != 4) return 0xFFFFFFFE;
        int rs1 = parse_register(tokens[1]);
        int rs2 = parse_register(tokens[2]);
        char* label = tokens[3];
        int label_addr = get_label_address(label);
        if (rs1 == -1 || rs2 == -1 || label_addr == -1) return 0xFFFFFFFE;
        int imm = label_addr - current_address;
        if (imm % 4 != 0) return 0xFFFFFFFE;  // ���� ���� Ȯ��


        // ���� ������ ó��
        if (imm < -2048 || imm > 2047) return 0xFFFFFFFE; // ��� �� ���� �ʰ�

        uint32_t imm12 = (imm >> 12) & 0x1;
        uint32_t imm10_5 = (imm >> 5) & 0x3F;
        uint32_t imm4_1 = (imm >> 1) & 0xF;
        uint32_t imm11 = (imm >> 11) & 0x1;

        uint32_t funct3;
        if (strcmp(tokens[0], "BEQ") == 0) { funct3 = 0x0; }
        else if (strcmp(tokens[0], "BNE") == 0) { funct3 = 0x1; }
        else if (strcmp(tokens[0], "BGE") == 0) { funct3 = 0x5; }
        else if (strcmp(tokens[0], "BLT") == 0) { funct3 = 0x4; }

        uint32_t machine_code = (imm12 << 31) | (imm10_5 << 25) |
            (rs2 << 20) | (rs1 << 15) |
            (funct3 << 12) | (imm4_1 << 8) |
            (imm11 << 7) | 0x63;
        return machine_code;
    }

    // J-type ��ɾ�: JAL
    else if (strcmp(tokens[0], "JAL") == 0) {
        if (token_count != 3) return 0xFFFFFFFE;
        int rd = parse_register(tokens[1]);
        char* label = tokens[2];
        int label_addr = get_label_address(label);
        if (rd == -1 || label_addr == -1) return 0xFFFFFFFE;
        int imm = label_addr - current_address;
        if (imm % 2 != 0) return 0xFFFFFFFE;  // �ݿ��� ���� Ȯ�� (JAL�� �ּ� 2����Ʈ ����)

        // JAL�� ��� ���� ����Ʈ �����̹Ƿ� ������ �ʽ��ϴ�.

        uint32_t imm20 = (imm >> 20) & 0x1;
        uint32_t imm10_1 = (imm >> 1) & 0x3FF;
        uint32_t imm11 = (imm >> 11) & 0x1;
        uint32_t imm19_12 = (imm >> 12) & 0xFF;

        uint32_t machine_code = (imm20 << 31) | (imm19_12 << 12) |
            (imm11 << 20) | (imm10_1 << 21) |
            (rd << 7) | 0x6F;
        return machine_code;
}
    // �������� �ʴ� ��ɾ�
    else {
        return 0xFFFFFFFE;
    }
}

// 32��Ʈ ������ ���� ���ڿ��� ��ȯ
void uint32_to_binary(uint32_t n, char* binary_str) {
    for (int i = 31; i >= 0; i--) {
        binary_str[31 - i] = (n & (1U << i)) ? '1' : '0';
    }
    binary_str[32] = '\0';
}

// ��� .o ���Ͽ� ����
int write_machine_code(char* o_filename) {
    FILE* ofp = fopen(o_filename, "w");
    if (ofp == NULL) return -1;

    for (int i = 0; i < instruction_count; i++) {
        if (instructions[i].machine_code == 0xFFFFFFFE) {
            fclose(ofp);
            return -1; // ���� ���� ����
        }
        char binary_str[33];
        uint32_to_binary(instructions[i].machine_code, binary_str);
        fprintf(ofp, "%s\n", binary_str);
    }

    fclose(ofp);
    return 0;
}

// ���� �ùķ��̼� �� .trace ���Ͽ� PC �� ����
int simulate_execution(char* trace_filename) {
    FILE* tfp = fopen(trace_filename, "w");
    if (tfp == NULL) return -1;

    uint32_t PC = 1000;  // PC�� 1000���� �ʱ�ȭ
    fprintf(tfp, "%u\n", PC);  // 10������ PC ���


    while (1) {
        // ��ɾ� �ε��� ���
        int index = (PC - 1000) / 4;
        if (index < 0 || index >= instruction_count) break;

        uint32_t inst = instructions[index].machine_code;
        if (inst == 0xFFFFFFFF) {  // EXIT ��ɾ�
          
            
            break;
        }

        // ��ɾ� ���ڵ� �� ����
        uint32_t opcode = inst & 0x7F;
        uint32_t rd, funct3, rs1, rs2, funct7, imm;

        switch (opcode) {
        case 0x33: { // R-type
            rd = (inst >> 7) & 0x1F;
            funct3 = (inst >> 12) & 0x7;
            rs1 = (inst >> 15) & 0x1F;
            rs2 = (inst >> 20) & 0x1F;
            funct7 = (inst >> 25) & 0x7F;

            if (funct3 == 0x0) {
                if (funct7 == 0x00) { // ADD
                    registers[rd] = registers[rs1] + registers[rs2];
                }
                else if (funct7 == 0x20) { // SUB
                    registers[rd] = registers[rs1] - registers[rs2];
                }
            }
            else if (funct3 == 0x7 && funct7 == 0x00) { // AND
                registers[rd] = registers[rs1] & registers[rs2];
            }
            else if (funct3 == 0x6 && funct7 == 0x00) { // OR
                registers[rd] = registers[rs1] | registers[rs2];
            }
            else if (funct3 == 0x4 && funct7 == 0x00) { // XOR
                registers[rd] = registers[rs1] ^ registers[rs2];
            }
            else if (funct3 == 0x1 && funct7 == 0x00) { // SLL
                registers[rd] = registers[rs1] << (registers[rs2] & 0x1F);
            }
            else if (funct3 == 0x5) {
                if (funct7 == 0x00) { // SRL
                    registers[rd] = registers[rs1] >> (registers[rs2] & 0x1F);
                }
                else if (funct7 == 0x20) { // SRA
                    registers[rd] = ((int32_t)registers[rs1]) >> (registers[rs2] & 0x1F);
                }
            }
            PC += 4;
            break;
        }
        case 0x13: { // I-type
            rd = (inst >> 7) & 0x1F;
            funct3 = (inst >> 12) & 0x7;
            rs1 = (inst >> 15) & 0x1F;
            imm = (inst >> 20) & 0xFFF;
            // �ñ״�ó Ȯ��
            if (imm & 0x800) imm |= 0xFFFFF000;

            if (funct3 == 0x0) { // ADDI
                registers[rd] = registers[rs1] + (int32_t)imm;
            }
            else if (funct3 == 0x7) { // ANDI
                registers[rd] = registers[rs1] & (int32_t)imm;
            }
            else if (funct3 == 0x6) { // ORI
                registers[rd] = registers[rs1] | (int32_t)imm;
            }
            else if (funct3 == 0x4) { // XORI
                registers[rd] = registers[rs1] ^ (int32_t)imm;
            }
            else if (funct3 == 0x1) { // SLLI
                uint32_t shamt = imm & 0x1F;
                registers[rd] = registers[rs1] << shamt;
            }
            else if (funct3 == 0x5) { // SRLI �Ǵ� SRAI
                uint32_t shamt = imm & 0x1F;
                uint32_t funct7 = (inst >> 25) & 0x7F;
                if (funct7 == 0x00) { // SRLI
                    registers[rd] = registers[rs1] >> shamt;
                }
                else if (funct7 == 0x20) { // SRAI
                    registers[rd] = ((int32_t)registers[rs1]) >> shamt;
                }
            }
            PC += 4;
            break;
        }
        case 0x03: { // LW
            rd = (inst >> 7) & 0x1F;
            funct3 = (inst >> 12) & 0x7;
            rs1 = (inst >> 15) & 0x1F;
            imm = (inst >> 20) & 0xFFF;
            // �ñ״�ó Ȯ��
            if (imm & 0x800) imm |= 0xFFFFF000;
            // �ùķ��̼ǿ����� �޸𸮸� 0���� ����
            registers[rd] = 0;
            PC += 4;
            break;
        }
        case 0x23: { // SW
            // �ùķ��̼ǿ����� �ƹ� ���۵� ���� ����
            PC += 4;
            break;
        }
        case 0x63: { // Branch ��ɾ�
            funct3 = (inst >> 12) & 0x7;
            rs1 = (inst >> 15) & 0x1F;
            rs2 = (inst >> 20) & 0x1F;
            imm = ((inst >> 31) & 0x1) << 12 |
                ((inst >> 25) & 0x3F) << 5 |
                ((inst >> 8) & 0xF) << 1 |
                ((inst >> 7) & 0x1) << 11;
            // �ñ״�ó Ȯ��
            if (imm & 0x1000) imm |= 0xFFFFE000;

            int branch = 0;
            if (funct3 == 0x0) { // BEQ
                if (registers[rs1] == registers[rs2]) branch = 1;
            }
            else if (funct3 == 0x1) { // BNE
                if (registers[rs1] != registers[rs2]) branch = 1;
            }
            else if (funct3 == 0x5) { // BGE
                if ((int32_t)registers[rs1] >= (int32_t)registers[rs2]) branch = 1;
            }
            else if (funct3 == 0x4) { // BLT
                if ((int32_t)registers[rs1] < (int32_t)registers[rs2]) branch = 1;
            }

            if (branch) {
                PC += imm * 1;  // ����: imm * 4���� ����
            }
            else {
                PC += 4;
            }
            break;
        }
        case 0x6F: { // JAL
            rd = (inst >> 7) & 0x1F;
            imm = ((inst >> 31) & 0x1) << 20 |
                ((inst >> 12) & 0xFF) << 12 |
                ((inst >> 20) & 0x1) << 11 |
                ((inst >> 21) & 0x3FF) << 1;
            // �ñ״�ó Ȯ��
            if (imm & 0x100000) imm |= 0xFFE00000;

            registers[rd] = PC + 4;
            PC += imm ;  // ����: imm * 4���� ����
            break;
        }
        case 0x67: { // JALR
            rd = (inst >> 7) & 0x1F;
            funct3 = (inst >> 12) & 0x7;
            rs1 = (inst >> 15) & 0x1F;
            imm = (inst >> 20) & 0xFFF;
            // �ñ״�ó Ȯ��
            if (imm & 0x800) imm |= 0xFFFFF000;

            registers[rd] = PC + 4;
            PC = (registers[rs1] + imm) & ~1;
            break;
        }
        default:

            break;
        }

        index = (PC - 1000) / 4;
        if (index < 0 || index >= instruction_count) break;

        fprintf(tfp, "%u\n", PC);  // PC�� 10������ ���

    }

    fclose(tfp);
    return 0;
}