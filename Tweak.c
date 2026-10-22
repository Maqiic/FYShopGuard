/*
 * FYShopGuard — standalone iOS TikTok Shop / feed ad filter.
 * Uses class/selector candidates inspected in TikTok 47.1.0. ASJTok 47.0.0 compatibility unverified.
 * EXPERIMENTAL: not tested on a real iPhone or verified to remove all ads.
 * No requests, no certificate handling, no network interception.
 * This source is intended to be linked into a .dylib for injection via ESign.
 *
 * Normal ads and Shop-linked creator videos have different labels.
 * This library checks explicit product and shop indicators, not just isAds.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ObjC runtime ABI only: avoids linking against CydiaSubstrate or Logos. */
typedef void *FYId;
typedef void *FYClass;
typedef void *FYSel;
typedef void *FYMethod;
typedef void (*FYIMP)(void);
extern FYClass objc_lookUpClass(const char *);
extern FYSel sel_registerName(const char *);
extern FYMethod class_getInstanceMethod(FYClass, FYSel);
extern FYIMP method_getImplementation(FYMethod);
extern FYIMP method_setImplementation(FYMethod, FYIMP);
extern FYId objc_msgSend(FYId, FYSel, ...);
extern FYId objc_getAssociatedObject(FYId, const void *);
extern void objc_setAssociatedObject(FYId, const void *, FYId, uintptr_t);
extern FYId objc_retain(FYId);
extern void objc_release(FYId);
extern void dispatch_async_f(void *, void *, void (*)(void *));
extern char _dispatch_main_q;

/* Objective-C association retain (nonatomic) policy. */
#define FY_RETAIN_NONATOMIC 1
static const char fy_raw_ad_marker;
static const char fy_raw_shop_marker;
static const char fy_skip_pending_marker;

static FYId send_id(FYId o, const char *name) {
    if (!o) return NULL;
    FYSel s = sel_registerName(name);
    if (!((bool (*)(FYId,FYSel,FYSel))objc_msgSend)(o,sel_registerName("respondsToSelector:"),s)) return NULL;
    return ((FYId (*)(FYId,FYSel))objc_msgSend)(o,s);
}
static FYId send_arg(FYId o, const char *name, FYId arg) {
    if (!o) return NULL;
    return ((FYId (*)(FYId,FYSel,FYId))objc_msgSend)(o,sel_registerName(name),arg);
}
static bool responds(FYId o, const char *name) {
    return o && ((bool (*)(FYId,FYSel,FYSel))objc_msgSend)(o,sel_registerName("respondsToSelector:"),sel_registerName(name));
}
static long send_integer(FYId o, const char *name) {
    if (!responds(o,name)) return 0;
    return ((long (*)(FYId,FYSel))objc_msgSend)(o,sel_registerName(name));
}
static bool send_bool(FYId o, const char *name) {
    if (!responds(o,name)) return false;
    return ((bool (*)(FYId,FYSel))objc_msgSend)(o,sel_registerName(name));
}
static FYId text(const char *name) {
    FYClass nsstring = objc_lookUpClass("NSString");
    if (!nsstring) return NULL;
    return ((FYId (*)(FYId,FYSel,const char *))objc_msgSend)((FYId)nsstring,sel_registerName("stringWithUTF8String:"),name);
}
static bool nonempty(FYId x) {
    if (!x || (!responds(x,"length") && !responds(x,"count"))) return false;
    return send_integer(x,responds(x,"length") ? "length" : "count") > 0;
}
static bool truthy(FYId x) {
    if (!x || !responds(x,"boolValue")) return false;
    return send_bool(x,"boolValue");
}
static FYId dict_value(FYId dict,const char *k) {
    return responds(dict,"objectForKey:") ? send_arg(dict,"objectForKey:",text(k)) : NULL;
}

/*
 * Shop-linked creator posts are often NOT flagged as ads.
 * Restrict detection to explicit product, shopping and goods-seeding links.
 * Do not treat every ordinary video anchor or generic "commerce_info" as Shop.
 * This is heuristic because TikTok's server schema is not stable.
 */
static bool kind_of(FYId obj, const char *className) {
    FYClass c=objc_lookUpClass(className);
    return obj && c && ((bool (*)(FYId,FYSel,FYClass))objc_msgSend)(obj,sel_registerName("isKindOfClass:"),c);
}
static bool object_present(FYId obj) {
    if (!obj) return false;
    if (responds(obj,"count") || responds(obj,"length")) return nonempty(obj);
    if (responds(obj,"boolValue")) return truthy(obj);
    return true;
}
static bool url_mentions_shop(FYId x) {
    if (!kind_of(x,"NSString")) return false;
    FYId lower=send_id(x,"lowercaseString");
    if (!lower) return false;
    const char *tokens[]={"tiktokshop", "ecom", "ec_product", "product_detail", "product_id", "shop_anchor", "goods_seeding", "shopify"};
    for (unsigned i=0;i<sizeof(tokens)/sizeof(tokens[0]);i++) {
        FYId key=text(tokens[i]);
        if (!key) continue;
        if (((bool (*)(FYId,FYSel,FYId))objc_msgSend)(lower,sel_registerName("containsString:"),key)) return true;
    }
    return false;
}
static bool has_shop_fields(FYId dict) {
    if (!responds(dict,"objectForKey:")) return false;
    const char *keys[]={"product_id", "product_ids", "product_info", "product_list", "products", "shop_id", "shop_info", "shop_anchor", "shop_product", "ecom_product", "ec_product", "goods_seeding_info", "shopping_info"};
    for (unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)
        if (object_present(dict_value(dict,keys[i]))) return true;
    return false;
}
static bool anchor_has_shop(FYId a) {
    if (!a) return false;
    if (kind_of(a,"AWEAwemeAnchorGoodsSeedingModel")) return true;
    if (responds(a,"objectForKey:")) {
        if (has_shop_fields(a)) return true;
        const char *keys[]={"schema", "url", "open_url", "anchor_url", "link", "extra", "keyword"};
        for (unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)
            if (url_mentions_shop(dict_value(a,keys[i]))) return true;
    } else {
        if (object_present(send_id(a,"productId")) || object_present(send_id(a,"shopAnchor")) || object_present(send_id(a,"productAnchor"))) return true;
        const char *keys[]={"schema", "url", "anchorUrl", "openURL", "extra"};
        for (unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)
            if (url_mentions_shop(send_id(a,keys[i]))) return true;
    }
    return false;
}
static bool anchors_have_shop(FYId anchors) {
    if (!anchors) return false;
    if (anchor_has_shop(anchors)) return true;
    if (!responds(anchors,"count") || !responds(anchors,"objectAtIndex:")) return false;
    long n=send_integer(anchors,"count");
    if (n<1 || n>48) return false;
    for (long i=0;i<n;i++) {
        FYId item=((FYId (*)(FYId,FYSel,unsigned long))objc_msgSend)(anchors,sel_registerName("objectAtIndex:"),(unsigned long)i);
        if (anchor_has_shop(item)) return true;
    }
    return false;
}
static bool is_raw_shop(FYId dict) {
    if (!responds(dict,"objectForKey:")) return false;
    const char *flags[]={"is_ecom", "is_shop_video", "is_shopping_video", "is_product_video"};
    for (unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if (truthy(dict_value(dict,flags[i]))) return true;
    if (has_shop_fields(dict)) return true;
    const char *nested[]={"commerce_info", "ecommerce_info", "ecom_info", "shop_data", "commerce_model"};
    for (unsigned i=0;i<sizeof(nested)/sizeof(nested[0]);i++)
        if (has_shop_fields(dict_value(dict,nested[i]))) return true;
    const char *anchors[]={"anchors", "anchor_infos", "anchor_info", "anchor_list", "anchorInfos"};
    for (unsigned i=0;i<sizeof(anchors)/sizeof(anchors[0]);i++)
        if (anchors_have_shop(dict_value(dict,anchors[i]))) return true;
    return false;
}

/* Raw server fields are inspected before VibeTok mutates the model. */
static bool is_raw_ad(FYId d) {
    if (!responds(d,"objectForKey:")) return false;
    const char *flags[] = {"is_ad","is_ads","is_pseudo_ad","is_ads_or_pseudo_ads", "is_ad_virtual"};
    for (unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if (truthy(dict_value(d,flags[i]))) return true;
    const char *numbers[] = {"ad_type","ad_aweme_source"};
    for (unsigned i=0;i<sizeof(numbers)/sizeof(numbers[0]);i++) {
        FYId x = dict_value(d,numbers[i]);
        if (x && responds(x,"integerValue") && send_integer(x,"integerValue")>0) return true;
    }
    const char *objects[] = {"ad_info","ad_id","raw_ad","ad_data"};
    for (unsigned i=0;i<sizeof(objects)/sizeof(objects[0]);i++)
        if (nonempty(dict_value(d,objects[i]))) return true;
    return false;
}

static bool is_aweme(FYId a) {
    if (!a) return false;
    FYClass cls = objc_lookUpClass("AWEAwemeModel");
    return cls && ((bool (*)(FYId,FYSel,FYClass))objc_msgSend)(a,sel_registerName("isKindOfClass:"),cls);
}
static bool is_ad(FYId a) {
    if (!is_aweme(a)) return false;
    if (objc_getAssociatedObject(a,&fy_raw_ad_marker)) return true;
    if (send_bool(a,"isAdsOrPseudoAds") || send_bool(a,"isAds") || send_bool(a,"isAd")) return true;
    if (send_integer(a,"adType")>0 || send_integer(a,"adSource")>0) return true;
    const char *models[]={"adInfo","adInfoModel","adOrPseudoAdModel"};
    for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);i++)
        if (send_id(a,models[i])) return true;
    return false;
}

static bool is_shop(FYId a) {
    if (!is_aweme(a)) return false;
    if (objc_getAssociatedObject(a,&fy_raw_shop_marker)) return true;
    if (send_bool(a,"isEcom") || send_bool(a,"isShopVideo")) return true;
    const char *models[]={"shopAnchor", "productAnchor", "shopProduct", "productInfo", "productList"};
    for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);i++)
        if (object_present(send_id(a,models[i]))) return true;
    const char *anchors[]={"anchorInfos", "anchorInfo", "anchors", "anchorList"};
    for (unsigned i=0;i<sizeof(anchors)/sizeof(anchors[0]);i++)
        if (anchors_have_shop(send_id(a,anchors[i]))) return true;
    const char *commerce[]={"commerceInfo", "commerceModel", "ecommerceInfo"};
    for (unsigned i=0;i<sizeof(commerce)/sizeof(commerce[0]);i++) {
        FYId c=send_id(a,commerce[i]);
        if (has_shop_fields(c)) return true;
        if (object_present(send_id(c,"productId")) || object_present(send_id(c,"shopId")) ||
            object_present(send_id(c,"productList")) || object_present(send_id(c,"shopAnchor"))) return true;
    }
    return false;
}
static bool is_blocked(FYId a) {return is_ad(a) || is_shop(a);}

/* Hook #1: capture advert attributes straight from server JSON. */
static FYId (*original_aweme_init)(FYId,FYSel,FYId,FYId);
static FYId aweme_init(FYId self,FYSel cmd,FYId dict,FYId error) {
    bool marked = is_raw_ad(dict);
    bool shop = is_raw_shop(dict);
    FYId result = original_aweme_init ? original_aweme_init(self,cmd,dict,error):NULL;
    if ((marked || shop) && result) {
        FYClass num = objc_lookUpClass("NSNumber");
        if (num) {
            FYId yes = ((FYId (*)(FYId,FYSel,bool))objc_msgSend)((FYId)num,sel_registerName("numberWithBool:"),true);
            if (yes && marked) objc_setAssociatedObject(result,&fy_raw_ad_marker,yes,FY_RETAIN_NONATOMIC);
            if (yes && shop) objc_setAssociatedObject(result,&fy_raw_shop_marker,yes,FY_RETAIN_NONATOMIC);
        }
    }
    return result;
}

/* Hook #2: filter flagged ads and Shop-linked Aweme models from feed collections. */
typedef void (*FYListSetter)(FYId,FYSel,FYId);
struct ListHook {const char *cls; const char *sel; FYListSetter original;};
static struct ListHook list_hooks[] = {
    {"AWEAwemeResponseModel", "setAwemeList:",NULL},
    {"AWEAwemeMultiResponseModel", "setAwemeList:",NULL},
    {"AWEFreshAwemeResponseModel", "setAwemeList:",NULL},
    {"AWEFeedTableViewController", "setAwemeModels:",NULL}
};
static FYId filtered_list(FYId list) {
    if (!list || !responds(list,"count") || !responds(list,"objectAtIndex:") || !responds(list,"mutableCopy")) return NULL;
    long count = send_integer(list,"count");
    if (count < 1 || count > 500) return NULL;
    FYId copy = NULL;
    for (long i=count-1;i>=0;i--) {
        FYId item = ((FYId (*)(FYId,FYSel,unsigned long))objc_msgSend)(list,sel_registerName("objectAtIndex:"),(unsigned long)i);
        if (is_blocked(item)) {
            if (!copy) copy = send_id(list,"mutableCopy");
            if (copy) ((void (*)(FYId,FYSel,unsigned long))objc_msgSend)(copy,sel_registerName("removeObjectAtIndex:"),(unsigned long)i);
        }
    }
    return copy;
}
static void list_setter_common(unsigned slot,FYId self,FYSel cmd,FYId items) {
    FYId result = filtered_list(items);
    FYListSetter original = list_hooks[slot].original;
    if (original) original(self,cmd,result ? result:items);
    if (result) objc_release(result);
}
static void list_0(FYId self,FYSel cmd,FYId items){list_setter_common(0,self,cmd,items);}
static void list_1(FYId self,FYSel cmd,FYId items){list_setter_common(1,self,cmd,items);}
static void list_2(FYId self,FYSel cmd,FYId items){list_setter_common(2,self,cmd,items);}
static void list_3(FYId self,FYSel cmd,FYId items){list_setter_common(3,self,cmd,items);}

/* Hook #3: feed items inserted after parsing get a render-time fallback.
   Unlike older approaches, we don't require a controller to implement both
   currentAweme AND scrollToNextVideo; they may live in different parents. */
static void (*original_display)(FYId,FYSel,long);
struct FYSkip {FYId cell, feed, aweme;};
static FYId find_cell_aweme(FYId cell) {
    const char *candidates[]={"awemeModel","aweme","currentAweme","currentAwemeModel","model","videoAweme"};
    for (unsigned i=0;i<sizeof(candidates)/sizeof(candidates[0]);i++) {
        FYId x=send_id(cell,candidates[i]);
        if (is_aweme(x)) return x;
        FYId nested=send_id(x,"awemeModel");
        if (is_aweme(nested)) return nested;
    }
    return NULL;
}
static FYId find_feed(FYId cell) {
    FYId node = cell;
    for (int i=0;node && i<18;i++) {
        if (responds(node,"scrollToNextVideo") || responds(node,"scrollToNextItem")) return node;
        node=send_id(node,"parentViewController");
    }
    return NULL;
}
static void skip_on_main(void *ctx) {
    struct FYSkip *s=(struct FYSkip *)ctx;
    FYId visible = find_cell_aweme(s->cell);
    if (!visible) {
        visible=send_id(s->feed,"currentAweme");
        if (!is_aweme(visible)) visible=send_id(s->feed,"currentAwemeModel");
    }
    if (visible == s->aweme && is_blocked(visible)) {
        if (responds(s->feed,"scrollToNextVideo"))
            ((void (*)(FYId,FYSel))objc_msgSend)(s->feed,sel_registerName("scrollToNextVideo"));
        else if (responds(s->feed,"scrollToNextItem"))
            ((void (*)(FYId,FYSel))objc_msgSend)(s->feed,sel_registerName("scrollToNextItem"));
    }
    objc_setAssociatedObject(s->cell,&fy_skip_pending_marker,NULL,FY_RETAIN_NONATOMIC);
    objc_release(s->cell);objc_release(s->feed);objc_release(s->aweme);
    extern void free(void *);
    free(s);
}
static void cell_display(FYId self,FYSel cmd,long reason) {
    if(original_display) original_display(self,cmd,reason);
    if (objc_getAssociatedObject(self,&fy_skip_pending_marker)) return;
    FYId aweme=find_cell_aweme(self);
    FYId feed=find_feed(self);
    if (!aweme && feed) {
        aweme=send_id(feed,"currentAweme");
        if (!is_aweme(aweme)) aweme=send_id(feed,"currentAwemeModel");
    }
    if (!is_blocked(aweme) || !feed) return;
    extern void *malloc(unsigned long);
    struct FYSkip *s=(struct FYSkip *)malloc(sizeof(*s));
    if (!s) return;
    s->cell=objc_retain(self);s->feed=objc_retain(feed);s->aweme=objc_retain(aweme);
    FYClass n=objc_lookUpClass("NSNumber");
    if (n) {
        FYId yes=((FYId (*)(FYId,FYSel,bool))objc_msgSend)((FYId)n,sel_registerName("numberWithBool:"),true);
        if (yes) objc_setAssociatedObject(self,&fy_skip_pending_marker,yes,FY_RETAIN_NONATOMIC);
    }
    dispatch_async_f((void *)&_dispatch_main_q,s,skip_on_main);
}

static void install_once(void) {
    static bool installed=false;
    if(installed) return;
    /* Check the two critical classes before installing any hook. */
    FYClass aweme=objc_lookUpClass("AWEAwemeModel");
    FYClass cell=objc_lookUpClass("AWEFeedCellViewController");
    if (!aweme || !cell) return;
    FYMethod init=class_getInstanceMethod(aweme,sel_registerName("initWithDictionary:error:"));
    FYMethod display=class_getInstanceMethod(cell,sel_registerName("containerDidFullyDisplayWithReason:"));
    if(!init || !display) return;
    installed=true;
    original_aweme_init=(FYId(*)(FYId,FYSel,FYId,FYId))method_setImplementation(init,(FYIMP)aweme_init);
    original_display=(void(*)(FYId,FYSel,long))method_setImplementation(display,(FYIMP)cell_display);
    FYIMP replacements[]={ (FYIMP)list_0,(FYIMP)list_1,(FYIMP)list_2,(FYIMP)list_3 };
    for(unsigned i=0;i<sizeof(list_hooks)/sizeof(list_hooks[0]);i++) {
        FYClass cls=objc_lookUpClass(list_hooks[i].cls);
        if (!cls) continue;
        FYMethod setter=class_getInstanceMethod(cls,sel_registerName(list_hooks[i].sel));
        if (!setter) continue;
        /* If inherited, avoid swapping the superclass method globally. */
        extern FYMethod *class_copyMethodList(FYClass,unsigned *);
        unsigned n=0;FYMethod *own=class_copyMethodList(cls,&n);bool owns=false;
        for(unsigned j=0;j<n;j++) if(own[j]==setter){owns=true;break;}
        extern void free(void *);
        if(own) free(own);
        if(owns) list_hooks[i].original=(FYListSetter)method_setImplementation(setter,replacements[i]);
    }
}
__attribute__((constructor)) static void start_fy_filter(void) {
    /* Defer installation until a main-loop turn. Hook functions chain to the
       IMP previously installed by the TikTok app or another tweak. */
    dispatch_async_f((void *)&_dispatch_main_q,NULL,(void(*)(void*))install_once);
}
