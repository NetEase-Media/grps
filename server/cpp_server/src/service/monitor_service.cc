/*
 * Copyright 2022 netease. All rights reserved.
 * Author zhaochaochao@corp.netease.com
 * Date   2023/09/13
 * Brief  monitor service.
 */

#include "monitor_service.h"

#include <brpc/server.h>

#include "monitor/monitor.h"

namespace netease::grps {
void MonitorServiceImpl::Metrics(::google::protobuf::RpcController* controller,
                                 const ::grps::protos::v1::EmptyGrpsMessage* request,
                                 ::grps::protos::v1::EmptyGrpsMessage* response,
                                 ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto* cntl = (brpc::Controller*)controller;

  if (cntl->http_request().method() == brpc::HTTP_METHOD_GET) {
    cntl->http_response().set_content_type("text/html");
    cntl->response_attachment().append(
      "<!DOCTYPE html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><script "
      "language=\"javascript\" type=\"text/javascript\" src=\"/grps/v1/js/jquery_min\"></script><script "
      "language=\"javascript\" type=\"text/javascript\" src=\"/grps/v1/js/flot_min\"></script><script "
      "type=\"text/javascript\">$(function(){$(\".tabs-menu "
      "li\").click(function(i){window.location.href=$(this).attr(\"id\")})})</script><style "
      "type=\"text/"
      "css\">#layer1{margin:0;padding:0;width:1111px}.variable{margin:0;color:#000;cursor:pointer;position:relative;"
      "background-color:#fff}.nonplot-variable{margin:0;color:#000;position:relative;background-color:#fff}p{padding:"
      "2px "
      "0;margin:0}.detail{margin:0;width:800px;background-color:#fafafa}.flot-placeholder{width:800px;height:200px;"
      "line-height:1.2em}</style><script type=\"text/javascript\">var "
      "everEnabled={},enabled={},hovering_var=\"\",timeoutId={},lastPlot={};function "
      "prepareGraphs(){$(\".detail\").hide(),$(\".variable\").click(function(){var "
      "e=$(this).next(\".detail\");e.slideToggle(\"fast\");var "
      "i=e.children(\":first-child\").attr(\"id\");everEnabled[i]||(everEnabled[i]=!0,$(\"<div "
      "id='tooltip-\"+i+\"'></div>\").css({position:\"absolute\",display:\"none\",border:\"1px solid "
      "#fdd\",padding:\"2px\",\"background-color\":\"#ffffca\",opacity:.8}).appendTo(\"body\"),$(\"#\"+i.replace(/"
      "[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/g,\"\\\\$&\")).bind(\"plothover\",function(e,a,t){var "
      "l,r,n;t?(null!=(l=lastPlot[hovering_var=i])&&(t.series.color=\"#808080\",l.draw()),r=t.datapoint[0],n=t."
      "datapoint[1],$(\"#tooltip-\"+i.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/g,\"\\\\$&\")).html(n+\"<br/"
      ">(\"+describeX(r,t.series)+\")\").css({top:t.pageY+5,left:t.pageX+15}).show()):(hovering_var=\"\",$(\"#tooltip-"
      "\"+i.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/g,\"\\\\$&\")).hide())}),$(\"#\"+i.replace(/"
      "[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/g,\"\\\\$&\")).bind(\"mouseleave\",function(){$(\"#tooltip-\"+i.replace(/"
      "[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\")).hide()})),enabled[i]?(enabled[i]=!1,clearTimeout(timeoutId[i])):(enabled[i]=!0,fetchData(i))}),$("
      "\".default_expand\").click()}var "
      "trendOptions={colors:[\"#F0D06E\",\"#F0B06E\",\"#F0A06E\",\"#F0906E\",\"#F0806E\"],legend:{show:!1},grid:{"
      "hoverable:!0},xaxis:{ticks:[[29,\"-1 day\"],[53,\"-1 hour\"],[113,\"-1 "
      "minute\"]]}},cdfOptions={grid:{hoverable:!0},lines:{show:!0,fill:!0},xaxis:{ticks:[[10,\"10%\"],[20,\"20%\"],["
      "30,\"30%\"],[40,\"40%\"],[50,\"50%\"],[60,\"60%\"],[70,\"70%\"],[80,\"80%\"],[90,\"90%\"],[101,\"99.99%\"]]}};"
      "function describeTrendX(e){return 173<=e?\"just now\":113<e?e-173+\" second\":53<e?e-114+\" "
      "minute\":29<e?e-54+\" hour\":e-30+\" day\"}function describeCDFX(e){return "
      "e<=99?e+\"%\":100==e?\"99.9%\":101==e?\"99.99%\":\"unknown \"+e}function describeX(e,a){return "
      "173==a.data[a.data.length-1][0]?null!=a.label?a.label+\" "
      "\"+describeTrendX(e):describeTrendX(e):101==a.data[a.data.length-1][0]?describeCDFX(e):e}function "
      "fetchData(r){$.ajax({url:\"/grps/v1/monitor/"
      "series?name=\"+r.replaceAll(\"\\\\\",\"\"),type:\"GET\",dataType:\"json\",success:function(e){if(hovering_var!="
      "r)if(\"trend\"==e.label)lastPlot[r]=$.plot(\"#\"+r.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\"),[e.data],trendOptions),$(\"#value-\"+r.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\")).html(e.data[e.data.length-1][1]);else "
      "if(\"cdf\"==e.label)lastPlot[r]=$.plot(\"#\"+r.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\"),[e.data],cdfOptions),$(\"#value-\"+r.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\")).html(e.data[e.data.length-1][1]);else{lastPlot[r]=$.plot(\"#\"+r.replace(/[!\"#$%&'()*+,./"
      ":;<=>?@[\\\\\\]^`{|}~]/g,\"\\\\$&\"),e,trendOptions);for(var a='\"[',t=0;t<e.length;++t){0!=t&&(a+=\",\");var "
      "l=e[t].data;a+=l[l.length-1][1]}a+=']\"',$(\"#value-\"+r.replace(/[!\"#$%&'()*+,./:;<=>?@[\\\\\\]^`{|}~]/"
      "g,\"\\\\$&\")).html(a)}}}),enabled[r]&&(timeoutId[r]=setTimeout(function(){fetchData(r)},1e3))}$(prepareGraphs)<"
      "/script></head><body></body></html>");
    cntl->response_attachment().append("<div id=\"layer1\">");
    cntl->response_attachment().append(
      "<strong style=\"font-size: 1.5em;\">>>click metrics name to start or stop showing animation<<</strong>");

    auto metrics_names = Monitor::Instance().GetMetricsNames();
    std::sort(metrics_names.begin(), metrics_names.end());
    for (const auto& name : metrics_names) {
      cntl->response_attachment().append("<p class=\"variable\">");
      cntl->response_attachment().append(name);
      cntl->response_attachment().append(" : <span id=\"value-");
      cntl->response_attachment().append(name);
      cntl->response_attachment().append("\"></span></p>");
      cntl->response_attachment().append("<div class=\"detail\">");
      cntl->response_attachment().append("<div id=\"");
      cntl->response_attachment().append(name);
      cntl->response_attachment().append(R"(" class="flot-placeholder"></div>)");
      cntl->response_attachment().append("</div>");
    }

    cntl->response_attachment().append("</div></body></html>");
    return;
  }
}

void MonitorServiceImpl::SeriesData(::google::protobuf::RpcController* controller,
                                    const ::grps::protos::v1::EmptyGrpsMessage* request,
                                    ::grps::protos::v1::EmptyGrpsMessage* response,
                                    ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto* cntl = (brpc::Controller*)controller;

  if (cntl->http_request().method() != brpc::HTTP_METHOD_GET) {
    cntl->http_response().set_status_code(brpc::HTTP_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  auto metrics_name = cntl->http_request().uri().GetQuery("name"); // Get "name" param from url
  if (metrics_name->empty()) {
    cntl->http_response().set_status_code(brpc::HTTP_STATUS_BAD_REQUEST);
    return;
  }

  // Set response
  cntl->http_response().set_content_type("application/json");
  cntl->response_attachment().append(Monitor::Instance().GetMetricsAggDataJson(metrics_name->c_str()));
}
} // namespace netease::grps