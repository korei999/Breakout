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
Parser::load(adt::String path)
{
    m_sName = path;
    if (m_lex.loadFile(m_pAlloc, path) == FAIL) return FAIL;

    m_tCurr = m_lex.next();
    m_tNext = m_lex.next();

    if ((m_tCurr.type != Token::LBRACE) && (m_tCurr.type != Token::LBRACKET))
    {
        CERR("wrong first token\n");
        return RESULT::FAIL;
    }

    return RESULT::OK;
}

RESULT
Parser::parse()
{
    do
    {
        m_aObjects.push(m_pAlloc, {});
        if (parseNode(this, &m_aObjects.last()) == FAIL)
        {
            LOG_WARN("parseNode() failed\n");
            return FAIL;
        }

    } while (m_tCurr.type == Token::LBRACE);

    return OK;
}

RESULT
Parser::loadParse(adt::String path)
{
    if (load(path) == RESULT::FAIL)
        return FAIL;

    parse();
    return OK;
}

static RESULT
expect(Parser* s, enum Token::TYPE t, adt::String svFile, int line)
{
    if (s->m_tCurr.type != t)
    {
        CERR("('{}', at {}): ({}): unexpected token: expected: '{}', got '{}'\n",
             svFile, line, s->m_sName, char(t), char(s->m_tCurr.type));
        return FAIL;
    }

    return OK;
}

static void
next(Parser* s)
{
    s->m_tCurr = s->m_tNext;
    s->m_tNext = s->m_lex.next();
}

static RESULT
parseNode(Parser* s, Object* pNode)
{
    switch (s->m_tCurr.type)
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
    *pTV = {.tag = TAG::STRING, .val {.sv = s->m_tCurr.sLiteral}};
    next(s);
}

static void
parseNumber(Parser* s, TagVal* pTV)
{
    bool bReal = s->m_tCurr.sLiteral.lastOf('.') != adt::NPOS;

    if (bReal) *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(s->m_tCurr.sLiteral.m_pData)}};
    else *pTV = TagVal{.tag = TAG::LONG, .val = {.l = atol(s->m_tCurr.sLiteral.m_pData)}};

    next(s);
}

static RESULT
parseObject(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::VecBase<Object>(s->m_pAlloc, 1);
    auto& aObjs = getObject(pNode);

    for (; s->m_tCurr.type != Token::RBRACE; next(s))
    {
        OK_OR_RET(expect(s, Token::IDENT, ADT_LOGS_FILE, __LINE__));

        Object ob {.svKey = s->m_tCurr.sLiteral, .tagVal = {}};
        aObjs.push(s->m_pAlloc, ob);

        /* skip identifier and ':' */
        next(s);
        OK_OR_RET(expect(s, Token::ASSIGN, ADT_LOGS_FILE, __LINE__));
        next(s);

        OK_OR_RET(parseNode(s, &aObjs.last()));

        if (s->m_tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (aObjs.getSize() == 0) next(s);

    return OK;
}

static RESULT
parseArray(Parser* s, Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::VecBase<Object>(s->m_pAlloc, 1);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    for (; s->m_tCurr.type != Token::RBRACKET; next(s))
    {
        aTVs.push(s->m_pAlloc, {});

        switch (s->m_tCurr.type)
        {
            default:
            case Token::IDENT:
                parseIdent(s, &aTVs.last().tagVal);
                break;

            case Token::NULL_:
                parseNull(s, &aTVs.last().tagVal);
                break;

            case Token::TRUE_:
            case Token::FALSE_:
                parseBool(s, &aTVs.last().tagVal);
                break;

            case Token::NUMBER:
                parseNumber(s, &aTVs.last().tagVal);
                break;

            case Token::LBRACE:
                next(s);
                OK_OR_RET(parseObject(s, &aTVs.last()));
                break;
        }

        if (s->m_tCurr.type != Token::COMMA)
        {
            next(s);
            break;
        }
    }

    if (aTVs.getSize() == 0) next(s);

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
    bool b = s->m_tCurr.type == Token::TRUE_ ? true : false;
    *pTV = {.tag = TAG::BOOL, .val = {.b = b}};
    next(s);
}

void
Parser::destroy()
{
    auto fn = +[](Object* p, void* a) -> bool {
        auto* pAlloc = (IAllocator*)a;

        if (p->tagVal.tag == TAG::ARRAY || p->tagVal.tag == TAG::OBJECT)
            pAlloc->free(p->tagVal.val.a.m_pData);

        return false;
    };

    traverseAll(fn, m_pAlloc);
    m_lex.destroy(m_pAlloc);
    m_aObjects.destroy(m_pAlloc);
}

void
Parser::print(FILE* fp)
{
    for (auto& obj : m_aObjects)
    {
        printNode(fp, &obj, "", 0);
        fputc('\n', fp);
    }
}

void
printNode(FILE* fp, Object* pNode, adt::String svEnd, int depth)
{
    adt::String key = pNode->svKey;

    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::OBJECT:
            {
                auto& obj = getObject(pNode);
                adt::String q0, q1, objName0, objName1;

                if (key.m_size == 0)
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
                for (u32 i = 0; i < obj.getSize(); i++)
                {
                    adt::String slE = (i == obj.getSize() - 1) ? "\n" : ",\n";
                    printNode(fp, &obj[i], slE, depth + 2);
                }
                fprintf(fp, "%*s", depth, "");
                /*COUT("{: >{}}", "", depth);*/
                print::toFILE(fp, "}{}", svEnd);
            } break;

        case TAG::ARRAY:
            {
                auto& arr = getArray(pNode);
                adt::String q0, q1, arrName0, arrName1;

                if (key.m_size == 0)
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

                if (arr.getSize() == 0)
                {
                    print::toFILE(fp, "{}{}{}{}[", q0, arrName0, q1, arrName1);
                    print::toFILE(fp, "]{}", svEnd);
                    break;
                }

                print::toFILE(fp, "{}{}{}{}[\n", q0, arrName0, q1, arrName1);
                for (u32 i = 0; i < arr.getSize(); i++)
                {
                    adt::String slE = (i == arr.getSize() - 1) ? "\n" : ",\n";

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
                                printNode(fp, &arr[i], slE, depth + 2);
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
Parser::traverse(Object* pNode, bool (*pfn)(Object* p, void* pFnArgs), void* pArgs)
{
    switch (pNode->tagVal.tag)
    {
        default: break;

        case TAG::ARRAY:
        {
            auto& obj = getArray(pNode);

            for (u32 i = 0; i < obj.getSize(); i++)
                traverse(&obj[i], pfn, pArgs);
        }
        break;

        case TAG::OBJECT:
        {
            auto& obj = getObject(pNode);

            for (u32 i = 0; i < obj.getSize(); i++)
                traverse(&obj[i], pfn, pArgs);
        }
        break;
    }

    if (pfn(pNode, pArgs)) return;
}

adt::VecBase<Object>&
Parser::getRoot()
{
    assert(m_aObjects.m_size > 0 && "[Parser]: this json is empty");

    if (m_aObjects.m_size == 1) return getObject(&m_aObjects.first());
    else return m_aObjects;
}

} /* namespace json */
