#include "Parser.hh"

#include "adt/logs.hh"

using namespace adt;

namespace json
{

#define OK_OR_RET(RES) do { if (RES == RESULT::FAIL) return FAIL; } while(false)

static RESULT expect(Parser*s, enum Token::TYPE t, adt::String svFile, int line);
static void next(Parser* s);
static RESULT parseNode(Parser* s, Object* pNode);
static RESULT parseObject(Parser* s, Object* pNode);
static RESULT parseArray(Parser* s, Object* pNode); /* arrays are same as objects */
static void parseIdent(Parser* s, TagVal* pTV);
static void parseNumber(Parser* s, TagVal* pTV);
static void parseNull(Parser* s, TagVal* pTV);
static void parseBool(Parser* s, TagVal* pTV);

RESULT
ParserLoad(Parser* s, adt::String path)
{
    s->sName = path;
    if (LexerLoadFile(&s->l, path) == FAIL) return FAIL;

    s->tCurr = LexerNext(&s->l);
    s->tNext = LexerNext(&s->l);

    if ((s->tCurr.type != Token::LBRACE) && (s->tCurr.type != Token::LBRACKET))
    {
        CERR("wrong first token\n");
        return RESULT::FAIL;
    }

    return RESULT::OK;
}

RESULT
ParserParse(Parser* s)
{
    do
    {
        VecPush(&s->aObjects, s->pAlloc, {});
        if (parseNode(s, &VecLast(&s->aObjects)) == FAIL)
        {
            LOG_WARN("parseNode() failed\n");
            return FAIL;
        }

    } while (s->tCurr.type == Token::LBRACE);

    return OK;
}

RESULT
ParserLoadParse(Parser* s, adt::String path)
{
    if (ParserLoad(s, path) == RESULT::FAIL)
        return FAIL;

    ParserParse(s);
    return OK;
}

static RESULT
expect(Parser* s, enum Token::TYPE t, adt::String svFile, int line)
{
    if (s->tCurr.type != t)
    {
        CERR("('{}', at {}): ({}): unexpected token: expected: '{}', got '{}'\n",
             svFile, line, s->sName, char(t), char(s->tCurr.type));
        return FAIL;
    }

    return OK;
}

static void
next(Parser* s)
{
    s->tCurr = s->tNext;
    s->tNext = LexerNext(&s->l);
}

static RESULT
parseNode(Parser* s, Object* pNode)
{
    switch (s->tCurr.type)
    {
        default:
            next(s);
            break;

        case Token::IDENT:
            parseIdent(s, &pNode->tagVal);
            break;

        case Token::NUMBER:
            parseNumber(s, &pNode->tagVal);
            break;

        case Token::LBRACE:
            next(s); /* skip brace */
            OK_OR_RET(parseObject(s, pNode));
            break;

        case Token::LBRACKET:
            next(s); /* skip bracket */
            OK_OR_RET(parseArray(s, pNode));
            break;

        case Token::NULL_:
            parseNull(s, &pNode->tagVal);
            break;

        case Token::TRUE_:
        case Token::FALSE_:
            parseBool(s, &pNode->tagVal);
            break;
    }

    return OK;
}

static void
parseIdent(Parser* s, TagVal* pTV)
{
    *pTV = {.tag = TAG::STRING, .val {.sv = s->tCurr.sLiteral}};
    next(s);
}

static void
parseNumber(Parser* s, TagVal* pTV)
{
    bool bReal = adt::StringLastOf(s->tCurr.sLiteral, '.') != adt::NPOS;

    if (bReal) *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(s->tCurr.sLiteral.pData)}};
    else *pTV = TagVal{.tag = TAG::LONG, .val = {.l = atol(s->tCurr.sLiteral.pData)}};

    next(s);
}

static RESULT
parseObject(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::VecBase<Object>(s->pAlloc, 1);
    auto& aObjs = getObject(pNode);

    for (; s->tCurr.type != Token::RBRACE; next(s))
    {
        OK_OR_RET(expect(s, Token::IDENT, ADT_LOGS_FILE, __LINE__));

        Object ob {.svKey = s->tCurr.sLiteral, .tagVal = {}};
        adt::VecPush(&aObjs, s->pAlloc, ob);

        /* skip identifier and ':' */
        next(s);
        OK_OR_RET(expect(s, Token::ASSIGN, ADT_LOGS_FILE, __LINE__));
        next(s);

        OK_OR_RET(parseNode(s, &adt::VecLast(&aObjs)));

        if (s->tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (VecSize(&aObjs) == 0) next(s);

    return OK;
}

static RESULT
parseArray(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::VecBase<Object>(s->pAlloc, 1);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    for (; s->tCurr.type != Token::RBRACKET; next(s))
    {
        adt::VecPush(&aTVs, s->pAlloc, {});

        switch (s->tCurr.type)
        {
            default:
            case Token::IDENT:
                parseIdent(s, &adt::VecLast(&aTVs).tagVal);
                break;

            case Token::NULL_:
                parseNull(s, &adt::VecLast(&aTVs).tagVal);
                break;

            case Token::TRUE_:
            case Token::FALSE_:
                parseBool(s, &adt::VecLast(&aTVs).tagVal);
                break;

            case Token::NUMBER:
                parseNumber(s, &adt::VecLast(&aTVs).tagVal);
                break;

            case Token::LBRACE:
                next(s);
                OK_OR_RET(parseObject(s, &adt::VecLast(&aTVs)));
                break;
        }

        if (s->tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (VecSize(&aTVs) == 0) next(s);

    return OK;
}

static void
parseNull(Parser* s, TagVal* pTV)
{
    *pTV = {.tag = TAG::NULL_, .val = {nullptr}};
    next(s);
}

static void
parseBool(Parser* s, TagVal* pTV)
{
    bool b = s->tCurr.type == Token::TRUE_ ? true : false;
    *pTV = {.tag = TAG::BOOL, .val = {.b = b}};
    next(s);
}

void
ParserDestroy(Parser* s)
{
    auto fn = +[](Object* p, void* a) -> bool {
        auto* pAlloc = (IAllocator*)a;

        if (p->tagVal.tag == TAG::ARRAY || p->tagVal.tag == TAG::OBJECT)
            free(pAlloc, p->tagVal.val.a.pData);

        return false;
    };

    ParserTraverseAll(s, fn, s->pAlloc);
    LexerDestroy(&s->l);
    VecDestroy(&s->aObjects, s->pAlloc);
}

void
ParserPrint(Parser* s, FILE* fp)
{
    for (auto& obj : s->aObjects)
    {
        ParserPrintNode(fp, &obj, "", 0);
        fputc('\n', fp);
    }
}

void
ParserPrintNode(FILE* fp, Object* pNode, adt::String svEnd, int depth)
{
    adt::String key = pNode->svKey;

    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::OBJECT:
            {
                auto& obj = getObject(pNode);
                adt::String q0, q1, objName0, objName1;

                if (key.size == 0)
                {
                    q0 = q1 = objName1 = objName0 = "";
                }
                else
                {
                    objName0 = key;
                    objName1 = ": ";
                    q1 = q0 = "\"";
                }

                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "{}{}{}{}{\n", q0, objName0, q1, objName1);
                for (u32 i = 0; i < VecSize(&obj); i++)
                {
                    adt::String slE = (i == VecSize(&obj) - 1) ? "\n" : ",\n";
                    ParserPrintNode(fp, &obj[i], slE, depth + 2);
                }
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "}{}", svEnd);
            } break;

        case TAG::ARRAY:
            {
                auto& arr = getArray(pNode);
                adt::String q0, q1, arrName0, arrName1;

                if (key.size == 0)
                {
                    q0 =  q1 = arrName1 = arrName0 = "";
                }
                else
                {
                    arrName0 = key;
                    arrName1 = ": ";
                    q1 = q0 = "\"";
                }

                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/

                if (VecSize(&arr) == 0)
                {
                    print::toFILE(fp, "{}{}{}{}[", q0, arrName0, q1, arrName1);
                    print::toFILE(fp, "]{}", svEnd);
                    break;
                }

                print::toFILE(fp, "{}{}{}{}[\n", q0, arrName0, q1, arrName1);
                for (u32 i = 0; i < VecSize(&arr); i++)
                {
                    adt::String slE = (i == VecSize(&arr) - 1) ? "\n" : ",\n";

                    switch (arr[i].tagVal.tag)
                    {
                        default:
                        case TAG::STRING:
                            {
                                adt::String sl = getString(&arr[i]);
                                fprintf(fp, "%*s", depth + 2, "");
                                /*COUT("{: >{}}", "", depth + 2);*/
                                print::toFILE(fp, "\"{}\"{}", sl, slE);
                            } break;

                        case TAG::NULL_:
                                fprintf(fp, "%*s", depth + 2, "");
                                /*COUT("{: >{}}", "", depth + 2);*/
                                print::toFILE(fp, "{}{}", "null", slE);
                            break;

                        case TAG::LONG:
                            {
                                long num = getLong(&arr[i]);
                                fprintf(fp, "%*s", depth + 2, "");
                                /*COUT("{: >{}}", "", depth + 2);*/
                                print::toFILE(fp, "{}{}", num, slE);
                            } break;

                        case TAG::DOUBLE:
                            {
                                double dnum = getDouble(&arr[i]);
                                fprintf(fp, "%*s", depth + 2, "");
                                /*COUT("{: >{}}", "", depth + 2);*/
                                print::toFILE(fp, "{}{}", dnum, slE);
                            } break;

                        case TAG::BOOL:
                            {
                                bool b = getBool(&arr[i]);
                                fprintf(fp, "%*s", depth + 2, "");
                                /*COUT("{: >{}}", "", depth + 2);*/
                                print::toFILE(fp, "{}{}", b ? "true" : "false", slE);
                            } break;

                        case TAG::OBJECT:
                                ParserPrintNode(fp, &arr[i], slE, depth + 2);
                            break;
                    }
                }
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "]{}", svEnd);
            } break;

        case TAG::DOUBLE:
            {
                double f = getDouble(pNode);
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "\"{}\": {}{}", key, f, svEnd);
            } break;

        case TAG::LONG:
            {
                long i = getLong(pNode);
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "\"{}\": {}{}", key, i, svEnd);
            } break;

        case TAG::NULL_:
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "\"{}\": {}{}", key, "null", svEnd);
            break;

        case TAG::STRING:
            {
                adt::String sl = getString(pNode);
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "\"{}\": \"{}\"{}", key, sl, svEnd);
            } break;

        case TAG::BOOL:
            {
                bool b = getBool(pNode);
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "\"{}\": {}{}", key, b ? "true" : "false", svEnd);
            } break;
    }
}

void
ParserTraverse(Parser*s, Object* pNode, bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
{
    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::ARRAY:
        case TAG::OBJECT: {
            auto& obj = getObject(pNode);

            for (u32 i = 0; i < VecSize(&obj); i++)
                ParserTraverse(s, &obj[i], pfn, pArgs);
        } break;
    }

    if (pfn(pNode, pArgs)) return;
}

adt::VecBase<Object>&
ParserGetRoot(Parser* s)
{
    assert(s->aObjects.size > 0 && "[Parser]: this json is empty");

    if (s->aObjects.size == 1) return getObject(&adt::VecFirst(&s->aObjects));
    else return s->aObjects;
}

} /* namespace json */
