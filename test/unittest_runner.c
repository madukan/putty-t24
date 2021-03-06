#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <inttypes.h>
#include "cmockery.h"

#include "t24.h"

void test_Bootstrap(void **state) {
  assert_int_equal(0LL, 0LL); 
}

void modalfatalbox(char *fmt, ...) {}

void t24_basic_highlight(termchar *newline, int cols, int jed_prefix_length);

termline *alloc_newline(const char* chars) {
  int cols = strlen(chars);
  termline *line;
  int j;

  line = snew(termline);
  line->chars = snewn(cols, termchar);
  for (j = 0; j < cols; j++) {
    line->chars[j].chr = chars[j];
    line->chars[j].attr = '.';
  }

  return line;
}

void freeline(termline *line) {
  if (line) {
    sfree(line->chars);
    sfree(line);
  }
}

termline* newline(const char* chars) {
  static termline* line = 0;
  if (line)
    freeline(line);
  line = alloc_newline(chars);
  return line;
}

const char* decode_colors(const termchar* chars, int cols) {
  int current_color = -1;
  int letter = 'a' - 1;
  static char buf[1024];
  int i;
  for (i = 0; i < cols; ++i) {
    int color = chars[i].attr;
    if (color == '.') {
      buf[i] = color;
      current_color = -1;
    } else {
      if (current_color != color) {
        current_color = color;
        letter += 1;
      }
      buf[i] = letter;
    }
  }
  buf[cols] = 0;
  return buf;
}

const char* syntax(const char *chars) {
  size_t sz = strlen(chars);
  termline* line = alloc_newline(chars);
  int jed_prefix_length = t24_is_jed_line(line->chars, sz);
  t24_basic_highlight(line->chars, sz, jed_prefix_length);
  const char* result = decode_colors(line->chars, sz);
  freeline(line);
  return result;
}

#define string_eq(a, b) assert_string_equal(syntax(a), b)

void test_Comments(void **state) {
  string_eq("1234 !--------", ".....aaaaaaaaa");
  string_eq("1234   !--------", ".......aaaaaaaaa");
  string_eq("1234 *--------", ".....aaaaaaaaa");
  string_eq("1234   *--------", ".....aaaaaaaaaaa");
  string_eq("1234 VAR ;* comments", ".........aaaaaaaaaaa");
  string_eq("1234 VAR  * comments", "..........a.........");
  string_eq("1234 VAR // comments", ".........aaaaaaaaaaa");
  string_eq("1234 A = '// string'  // slash/shash comment", 
            ".......a.bbbbbbbbbbb..cccccccccccccccccccccc");
  string_eq("1234 REM rem comment",
            ".....aaaaaaaaaaaaaaa");
  string_eq("1234   REM rem comment",
            ".......aaaaaaaaaaaaaaa");
  string_eq("1234   CALL REM ",
            ".......aaaa.....");
  string_eq("00427   WHILE ENV.MSG:REM.ENV.POS",
            "........aaaaa....................");
  string_eq("0052      YAC.BAL *= -1",
            "..................aa.bc");
  string_eq("0053      YAC.BAL *=",
            "..................aa");
  string_eq("0053      YAC.BAL *",
            "..................a");
}

void test_Ticket_dd6a19efa5_DATE(void **state) {
  string_eq("0029  ENTRIES<1, AC.STE.VALUE.DATE>    = TODAY", 
            "..............a........................b......");
  string_eq("0036  ENTRIES<1, AC.STE.BOOKING.DATE>  = TODAY", 
            "..............a........................b......");
  string_eq("0036  ENTRIES<1, AC.STE.TOOKING DATE>  = TODAY", 
            "..............a.................bbbb...c......");
  string_eq("0036 DATE = TODAY", 
            ".....aaaa.b......");
  string_eq("0036  DATE = TODAY", 
            "......aaaa.b......");
}

void test_Ticket_e8e02762a0_V_TIME(void **state) {
  string_eq("0017     V.TIME = ''", 
            "................a.bb");
  string_eq("0034     V.DELTA = TIME() - TIME1", 
            ".................a.bbbb...c......");
}

void test_Ticket_0bcfac1fb6_READNEXT_FROM(void **state) {
  string_eq("0167     READNEXT ID FROM 9 ELSE DONE = 1 ", 
            ".........aaaaaaaa....bbbb.c.dddd......e.f.");
}

void test_Ticket_3f3645bab6_UNTIL_DO_REPEAT(void **state) {
  string_eq("0204     UNTIL DONE DO ", 
            ".........aaaaa......bb.");
  string_eq("0230     REPEAT", 
            ".........aaaaaa");
  string_eq("0231     REPEAT ", 
            ".........aaaaaa.");
}

void test_Ticket_cce4f3f8dc1b52fc_F_IN_FILE(void **state) {
  string_eq("0015     OPENSEQ V.DIR.IN, V.FILE.IN TO F.IN.FILE ELSE", 
            ".........aaaaaaa.....................bb...........cccc");
}

void test_Ticket_0a8c43b9ed_AND_OR(void **state) {
  string_eq("0003 IF PAR.1 EQ 1 AND (PAR.2 GT 3 OR PAR.2 LE 0) THEN", 
            ".....aa.......bb.c.ddd........ee.f.gg.......hh.i..jjjj");
}

void test_Ticket_3bfb7be594de7fd6_ON(void **state) {
  string_eq("0053    WRITESEQ RECON.REC ON F.FILE ELSE", 
            "........aaaaaaaa...........bb........cccc");
}

void test_Ticket_2018f2fd4a6af18b_ON_ERROR(void **state) {
  string_eq("0186    WRITE R.RECON TO F.RECON, V.RECON.ID ON ERROR",
            "........aaaaa.........bb.....................cc.ddddd");
}

void test_Ticket_3359b57f89_INSERT(void **state) {
  string_eq("1234 $INSERT abc",
            ".....aaaaaaa....");
  string_eq("1234     $INSERT abc",
            ".........aaaaaaa....");
}

void test_Ticket_11ffe01a8e_Case_Sensitivity(void **state) {
  string_eq("0001 PROGRAm TRY.1",
            "..................");
  string_eq("0003 CRt '111'",
            ".........aaaaa");
}

void test_Ticket_f3edb7f0f64d34fa_CAPTURING(void **state) {
  string_eq("0014 EXECUTE \"ls\" CAPTURING DirListing",
            ".....aaaaaaa.bbbb.ccccccccc...........");
}

void test_Ticket_2f276877e490b371_Pluses_Line_number(void **state) {
  string_eq("++++   GOSUB GET.UNC.AMOUNT   ;* Comments",
            ".......aaaaa..................bbbbbbbbbbb");
  string_eq("++++++   GOSUB GET.UNC.AMOUNT   ;* Comments",
            ".........aaaaa..................bbbbbbbbbbb");
  string_eq("+++   GOSUB GET.UNC.AMOUNT   ;* Comments",
            "........................................");
}

void test_Ticket_5aa1f03d4dbecfc4_Plus_Minus_with_numbers(void **state) {
  string_eq("0003 X = -2", 
            ".......a.bc");
  string_eq("0004 A = X - 2", 
            ".......a...b.c");
  string_eq("0005 B = X-2", 
            ".......a..bc");
}

void test_Ticket_55be7a13e4_Star_Equal_comment(void **state) {
  string_eq("0007 A = 10 ;* also a comment", 
            ".......a.bb.ccccccccccccccccc");
  string_eq("0008 A = 2 ; * still comment", 
            ".......a.b.ccccccccccccccccc");
  string_eq("0011 C = A * B", 
            ".......a...b..");
  string_eq("0011    * comments", 
            ".....aaaaaaaaaaaaa");
  string_eq("0011 * comments", 
            ".....aaaaaaaaaa");
  string_eq("0013 C *= 5", 
            ".......aa.b");
  string_eq("0013   C *= 5", 
            ".........aa.b");
}

void test_Ticket_3ba17ef8f11c682b_Percent_in_name(void **state) {
  string_eq("0086   L%ANSWER = ''", 
            "................a.bb");
  string_eq("0078   IF L%ERR THEN", 
            ".......aa.......bbbb");
  string_eq("0088   L%PROMPT = 'aaa'", 
            "................a.bbbbb");
  string_eq("0089   CALL TXTINP(L%PROMPT, 8, 12, FM:'Y_N')", 
            ".......aaaa..................b..cc.....ddddd.");
}

void test_Ticket_b1934b4f87b4b5e0_Subroutine_in_name(void **state) {
  string_eq("0086   GOSUB CALL.SUBROUTINE", 
            ".......aaaaa................");
}

void test_Ticket_84e4f7ccaacbf303_Return_in_name(void **state) {
  string_eq("0064     COM RETURN.COMI.UNAMED", 
            ".........aaa...................");
  string_eq("0030     RETURN.CODE = \"\"",
            ".....................a.bb");
}

void test_Ticket_d107ffd1a45fa984_Great_equal(void **state) {
  string_eq("0245    IF PGM.NAME[1,2] >= 'G' BP.NO = 2 ELSE BP.NO = 1",
            "........aa..........b.c.ddddeee.......f.g.hhhh.......i.j");
  string_eq("0245    IF PGM.NAME[1,2] => 'G' BP.NO = 2 ELSE BP.NO = 1",
            "........aa..........b.c.ddddeee.......f.g.hhhh.......i.j");
  string_eq("0245    IF PGM.NAME[1,2] <= 'G' BP.NO = 2 ELSE BP.NO = 1",
            "........aa..........b.c.ddddeee.......f.g.hhhh.......i.j");
  string_eq("0245    IF PGM.NAME[1,2] =< 'G' BP.NO = 2 ELSE BP.NO = 1",
            "........aa..........b.c.ddddeee.......f.g.hhhh.......i.j");
}

void test_Ticket_d581209a49abc4a1_Common(void **state) {
  string_eq("0029    COMMON/SLCOM/F$SL.PARAM.FILE,    ;* SL",
            "........aaaaaa.bbbbb.....................ccccc");
  string_eq("0029    COMMON /SLCOM/F$SL.PARAM.FILE,    ;* SL",
            "........aaaaaa..bbbbb.....................ccccc");
  string_eq("0029    COM/SLCOM/F$SL.PARAM.FILE,    ;* SL",
            "........aaa.bbbbb.....................ccccc");
  string_eq("0029    COM /SLCOM/F$SL.PARAM.FILE,    ;* SL",
            "........aaa..bbbbb.....................ccccc");
  string_eq("0029    COM/ SLCOM/F$SL.PARAM.FILE,    ;* SL",
            "........aaa.bbbbbb.....................ccccc");
}

void test_Ticket_b37ddad3e7478348_Abss(void **state) {
  string_eq("0001   PROGRAM ABSS-TEST",
            ".......aaaaaaa..........");
  string_eq("0001   SUBROUTINE ABSS-TEST",
            ".......aaaaaaaaaa..........");
  string_eq("0001   GOSUB ABSS-TEST",
            ".......aaaaa..........");
  string_eq("0001   ABSS-TEST:",
            ".......aaaaaaaaaa");
}

void test_Ticket_a8f2fc089e9faa6f_Hash(void **state) {
  string_eq("0004    B = (A # 3)",
            "..........a....b.c.");
}

void test_Generic(void **state) {
  string_eq("0001 PROGRAM TRY.1",
            ".....aaaaaaa......");
}

void test_Execution_control(void **state) {
  string_eq("0001   GOSUB xxx",
            ".......aaaaa....");
  string_eq("0001   GOSUB xxx ",
            ".......aaaaa.....");
  string_eq("0001   RETURN",
            ".......aaaaaa");
  string_eq("0001   RETURN ",
            ".......aaaaaaa");
  string_eq("0001   STOP",
            ".......aaaa");
  string_eq("0001   STOP ",
            ".......aaaa.");
  string_eq("0001   GOTO xxx",
            ".......aaaa....");
  string_eq("0001   GOTO xxx ",
            ".......aaaa.....");
  string_eq("0001   EXECUTE xxx",
            ".......aaaaaaa....");
  string_eq("0001   EXECUTE xxx ",
            ".......aaaaaaa.....");
}

void test_Common(void **state) {
  string_eq("0000 A COMMON /BLOO.COMMON, ONE , TWO/ B$OO.ISIN.LIST, B$OO.SM.LST",
            ".......aaaaaa.b......................c............................");
  string_eq("0001 COMMON /BLOO.COMMON, ONE , TWO/ B$OO.ISIN.LIST, B$OO.SM.LST",
            ".....aaaaaa..bbbbbbbbbbbbbbbbbbbbbb.............................");
  string_eq("0001 COMMON /BLOO.COMMON/ B$OO.ISIN.LIST, B$OO.SM.LIST",
            ".....aaaaaa..bbbbbbbbbbb..............................");
}

void test_Hex(void **state) {
  string_eq("0001 ABCDEF",
            "...........");
  string_eq("0001 20202020",
            ".............");
  string_eq("0001 20202020   ",
            "................");
  string_eq("0001 20202020   X",
            ".....aaaaaaaa....");
  string_eq("0001 2",
            ".....a");
  string_eq("0001 2 ",
            ".....a.");
}

void test_Numbers(void **state) {
  string_eq("0001 A = 10",
            ".......a.bb");
  string_eq("0002 A = +10",
            ".......a.bcc");
  string_eq("0002 A = + 10",
            ".......a.b.cc");
  string_eq("0002 A = -10",
            ".......a.bcc");
  string_eq("0002 A = - 10",
            ".......a.b.cc");
}

void test_Relations(void **state) {
  string_eq("0002 A <> 10",
            ".......aa.bb");
  string_eq("0002 A := 10",
            ".......aa.bb");
  string_eq("0005 IF A GT B THEN CRT '1'",
            ".....aa...bb...cccc.ddd.eee");
  string_eq("0005 IF A <> B THEN CRT '2'",
            ".....aa...bb...cccc.ddd.eee");
  string_eq("0005 A<2> = 3",
            ".......a..b.c");
  string_eq("0005 IF A < B THEN CRT '3'",
            ".....aa..bbb..cccc.ddd.eee");
  string_eq("0005 IF A > B THEN CRT '3'",
            ".....aa..bbb..cccc.ddd.eee");
}

void test_Label(void **state) {
  string_eq("0017  LABEL:", 
            "......aaaaaa");
  string_eq("0018 LABEL: ", 
            ".....aaaaaa.");
  string_eq("0019    LABEL: ", 
            "........aaaaaa.");
  string_eq("0019    LA:EL: ", 
            "........aaa....");
  string_eq("0019    LABEL: a = b", 
            "........aaaaaa...b..");
  string_eq("0019    LABEL: a = b:c:b", 
            "........aaaaaa...b......");
  string_eq("0020    LABEL: // LABEL: ", 
            "........aaaaaa.bbbbbbbbbb");
}

void test_newline(void **state) {
  termline* line = newline("1000 X");
  assert_int_equal('1', line->chars[0].chr & 0xff);
  assert_int_equal('0', line->chars[1].chr & 0xff);
  assert_int_equal('0', line->chars[2].chr & 0xff);
  assert_int_equal('0', line->chars[3].chr & 0xff);
  assert_int_equal(' ', line->chars[4].chr & 0xff);
  assert_int_equal('X', line->chars[5].chr & 0xff);
}

#define assert_jed_line(line) t24_is_jed_line(newline(line)->chars, strlen(line))

void test_t24_is_jed_line(void **state) {
  assert_int_equal(0, assert_jed_line(""));
  assert_int_equal(0, assert_jed_line("100"));
  assert_int_equal(0, assert_jed_line("+++"));
  assert_int_equal(4, assert_jed_line("1000"));
  assert_int_equal(5, assert_jed_line("1000 X"));
  assert_int_equal(4, assert_jed_line("++++"));
  assert_int_equal(6, assert_jed_line("100000"));
  assert_int_equal(7, assert_jed_line("100000 "));
  assert_int_equal(7, assert_jed_line("100000 X"));
  assert_int_equal(6, assert_jed_line("++++++"));
  assert_int_equal(7, assert_jed_line("++++++ "));
  assert_int_equal(7, assert_jed_line("++++++ X"));
  assert_int_equal(7, assert_jed_line("++++++ ABCDEF"));
}

#define assert_t24_line(line) t24_is_t24_line(newline(line)->chars, strlen(line))

void test_t24_is_t24_line(void **) {
  assert_int_equal(0, assert_t24_line(""));
  assert_int_equal(0, assert_t24_line("------------------"));
  assert_int_equal(0, assert_t24_line("----------------------------------------"
                                      "--------------------"));
  assert_int_equal(1, assert_t24_line(" ---------------------------------------"
                                      "--------------------"));
}

int main(int argc, char* argv[]) {
  UnitTest tests[] = {
    unit_test(test_Bootstrap),
    unit_test(test_Comments),
    unit_test(test_Label),
    unit_test(test_Common),
    unit_test(test_Numbers),
    unit_test(test_Relations),
    unit_test(test_Generic),
    unit_test(test_Hex),
    unit_test(test_Ticket_dd6a19efa5_DATE),
    unit_test(test_Ticket_e8e02762a0_V_TIME),
    unit_test(test_Ticket_0bcfac1fb6_READNEXT_FROM),
    unit_test(test_Ticket_3f3645bab6_UNTIL_DO_REPEAT),
    unit_test(test_Ticket_cce4f3f8dc1b52fc_F_IN_FILE),
    unit_test(test_Ticket_0a8c43b9ed_AND_OR),
    unit_test(test_Ticket_3bfb7be594de7fd6_ON),
    unit_test(test_Ticket_2018f2fd4a6af18b_ON_ERROR),
    unit_test(test_Ticket_3359b57f89_INSERT),
    unit_test(test_Ticket_11ffe01a8e_Case_Sensitivity),
    unit_test(test_Ticket_f3edb7f0f64d34fa_CAPTURING),
    unit_test(test_Ticket_2f276877e490b371_Pluses_Line_number),
    unit_test(test_Ticket_5aa1f03d4dbecfc4_Plus_Minus_with_numbers),
    unit_test(test_Ticket_55be7a13e4_Star_Equal_comment),
    unit_test(test_Ticket_3ba17ef8f11c682b_Percent_in_name),
    unit_test(test_Ticket_b1934b4f87b4b5e0_Subroutine_in_name),
    unit_test(test_Ticket_84e4f7ccaacbf303_Return_in_name),
    unit_test(test_Ticket_d107ffd1a45fa984_Great_equal),
    unit_test(test_Ticket_d581209a49abc4a1_Common),
    unit_test(test_Ticket_b37ddad3e7478348_Abss),
    unit_test(test_Ticket_a8f2fc089e9faa6f_Hash),
    unit_test(test_Execution_control),
    unit_test(test_newline),
    unit_test(test_t24_is_jed_line),
    unit_test(test_t24_is_t24_line),
  };
  return run_tests(tests);
}
