`include "sd_defines.v"

module sd_controller_fifo_axi
  (
    input wire         S_AXI_ACLK,
    input wire         S_AXI_ARESETN,

    ////////////////////////////////////////////////////////////////////////////
    // AXI Lite Slave Interface
    input wire [31:0]  S_AXI_AWADDR,
    input wire         S_AXI_AWVALID,
    output wire        S_AXI_AWREADY,
    input wire [31:0]  S_AXI_WDATA,
    input wire [3:0]   S_AXI_WSTRB,
    input wire         S_AXI_WVALID,
    output wire        S_AXI_WREADY,
    output wire [1:0]  S_AXI_BRESP,
    output wire        S_AXI_BVALID,
    input wire         S_AXI_BREADY,

    input wire [31:0]  S_AXI_ARADDR,
    input wire         S_AXI_ARVALID,
    output wire        S_AXI_ARREADY,
    output wire [31:0] S_AXI_RDATA,
    output wire [1:0]  S_AXI_RRESP,
    output wire        S_AXI_RVALID,
    input wire         S_AXI_RREADY,

    ////////////////////////////////////////////////////////////////////////////
    // SDIO Interface

    output wire        SDIO_CLK,
    inout wire         SDIO_CMD,
    inout wire [3:0]   SDIO_DATA,
    input wire         SDIO_NODISK,

    input wire         sd_clk_i
    );

   wire                sd_cmd_dat_i;
   wire                sd_cmd_out_o;
   wire                sd_cmd_oe_o;

   wire [3:0]          sd_dat_dat_i;
   wire [3:0]          sd_dat_out_o;
   wire                sd_dat_oe_o;

   wire                sd_clk_o_pad;

   assign SDIO_CLK = sd_clk_o_pad;

   IOBUF
     #(
       .DRIVE(12), // Specify the output drive strength
       .IBUF_LOW_PWR("TRUE"), // Low Power - "TRUE", High Performance = "FALSE"
       .IOSTANDARD("DEFAULT"), // Specify the I/O standard
       .SLEW("SLOW") // Specify the output slew rate
       )
   IOBUF_CMD
     (
      .O(sd_cmd_dat_i),
      .IO(SDIO_CMD),
      .I(sd_cmd_out_o),
      .T(~sd_cmd_oe_o) // 3-state enable input, high=input, low=output
      );
   IOBUF
     #(
       .DRIVE(12), // Specify the output drive strength
       .IBUF_LOW_PWR("TRUE"), // Low Power - "TRUE", High Performance = "FALSE"
       .IOSTANDARD("DEFAULT"), // Specify the I/O standard
       .SLEW("SLOW") // Specify the output slew rate
       )
   IOBUF_DATA0
     (
      .O(sd_dat_dat_i[0]),
      .IO(SDIO_DATA[0]),
      .I(sd_dat_out_o[0]),
      .T(~sd_dat_oe_o) // 3-state enable input, high=input, low=output
      );
   IOBUF
     #(
       .DRIVE(12), // Specify the output drive strength
       .IBUF_LOW_PWR("TRUE"), // Low Power - "TRUE", High Performance = "FALSE"
       .IOSTANDARD("DEFAULT"), // Specify the I/O standard
       .SLEW("SLOW") // Specify the output slew rate
       )
   IOBUF_DATA1
     (
      .O(sd_dat_dat_i[1]),
      .IO(SDIO_DATA[1]),
      .I(sd_dat_out_o[1]),
      .T(~sd_dat_oe_o) // 3-state enable input, high=input, low=output
      );
   IOBUF
     #(
       .DRIVE(12), // Specify the output drive strength
       .IBUF_LOW_PWR("TRUE"), // Low Power - "TRUE", High Performance = "FALSE"
       .IOSTANDARD("DEFAULT"), // Specify the I/O standard
       .SLEW("SLOW") // Specify the output slew rate
       )
   IOBUF_DATA2
     (
      .O(sd_dat_dat_i[2]),
      .IO(SDIO_DATA[2]),
      .I(sd_dat_out_o[2]),
      .T(~sd_dat_oe_o) // 3-state enable input, high=input, low=output
      );
   IOBUF
     #(
       .DRIVE(12), // Specify the output drive strength
       .IBUF_LOW_PWR("TRUE"), // Low Power - "TRUE", High Performance = "FALSE"
       .IOSTANDARD("DEFAULT"), // Specify the I/O standard
       .SLEW("SLOW") // Specify the output slew rate
       )
   IOBUF_DATA3
     (
      .O(sd_dat_dat_i[3]),
      .IO(SDIO_DATA[3]),
      .I(sd_dat_out_o[3]),
      .T(~sd_dat_oe_o) // 3-state enable input, high=input, low=output
      );

`define tx_cmd_fifo 4'h0
`define rx_cmd_fifo 4'h1
`define tx_data_fifo 4'h2
`define rx_data_fifo 4'h3
`define status 4'h4
`define controll 4'h5
`define timer 4'h6

   assign sd_clk_o=sd_clk_i;
   assign sd_clk_o_pad  = sd_clk_i ;

   reg [2:0]           axist;

   reg [7:0]           controll_reg;
   reg [7:0]           status_reg;
   reg [9:0]           command_timeout_reg;

   reg [2:0]           wb_adr_i;
   wire [7:0]          wb_fifo_dat_i;
   wire [7:0]          wb_fifo_dat_o;
   reg [7:0]           wb_dat_i_storage;
   reg [7:0]           wb_dat_o_i;
   reg                 time_enable;

   wire [1:4]          fifo_full ;
   wire [1:4]          fifo_empty;
   wire                wb_fifo_we_i = (axist==3'b011)&(wb_adr_i[2]==1'b0);
//   wire                wb_fifo_re_i = (axist==3'b100)&(wb_adr_i[2]==1'b0);
   wire                wb_fifo_re_i = ( (axist==3'b000)&S_AXI_ARVALID&(S_AXI_ARADDR[2+2]==1'b0)&
                                        ((S_AXI_ARADDR[2+2:2]!=`rx_data_fifo)|~fifo_empty[4])
                                       |(axist==3'b101)&~fifo_empty[4]
                                       );
   wire [1:0]          sd_adr_o;
   wire [7:0]          sd_dat_o;
   wire [7:0]          sd_dat_i;

   assign wb_fifo_dat_i =wb_dat_i_storage;
   assign S_AXI_RDATA = (wb_adr_i[2]) ? {24'h0,wb_dat_o_i} : {24'h0,wb_fifo_dat_o} ;

   wire [1:0]          wb_fifo_adr_i = (axist==3'b000) ? S_AXI_ARADDR[3:2] : wb_adr_i[1:0];

   sd_fifo sd_fifo_0
     (
       .wb_adr_i(wb_fifo_adr_i[1:0]),
       .wb_dat_i(wb_fifo_dat_i),
       .wb_dat_o(wb_fifo_dat_o),
       .wb_we_i(wb_fifo_we_i),
       .wb_re_i(wb_fifo_re_i),
       .wb_clk(S_AXI_ACLK),
       .sd_adr_i(sd_adr_o),
       .sd_dat_i(sd_dat_o),
       .sd_dat_o(sd_dat_i),
       .sd_we_i(sd_we_o),
       .sd_re_i(sd_re_o),
       .sd_clk(sd_clk_o),
       .fifo_full(fifo_full),
       .fifo_empty(fifo_empty),
       .rst(~S_AXI_ARESETN) // | controll_reg[0])
       ) ;
   
   wire [1:0]          sd_adr_o_cmd;
   wire [7:0]          sd_dat_i_cmd;
   wire [7:0]          sd_dat_o_cmd;

   wire [1:0]          sd_adr_o_dat;
   wire [7:0]          sd_dat_i_dat;
   wire [7:0]          sd_dat_o_dat;
   wire [1:0]          st_dat_t;

   sd_cmd_phy sdc_cmd_phy_0
     (
      .sd_clk(sd_clk_o),
      .rst(~S_AXI_ARESETN),//| controll_reg[0]),
      .cmd_dat_i(sd_cmd_dat_i),
      .cmd_dat_o(sd_cmd_out_o),
      .cmd_oe_o(sd_cmd_oe_o),
      .sd_adr_o(sd_adr_o_cmd),
      .sd_dat_i(sd_dat_i_cmd),
      .sd_dat_o(sd_dat_o_cmd),
      .sd_we_o(sd_we_o_cmd),
      .sd_re_o(sd_re_o_cmd),
      .fifo_full(fifo_full[1:2]),
      .fifo_empty(fifo_empty[1:2]),
      .start_dat_t(st_dat_t),
      .fifo_acces_token(fifo_acces_token)
      );


   sd_data_phy sd_data_phy_0
     (
      .sd_clk(sd_clk_o),
      .rst(~S_AXI_ARESETN | controll_reg[0]),
      .DAT_oe_o( sd_dat_oe_o),
      .DAT_dat_o(sd_dat_out_o),
      .DAT_dat_i(sd_dat_dat_i),
      .sd_adr_o(sd_adr_o_dat),
      .sd_dat_i(sd_dat_i_dat),
      .sd_dat_o(sd_dat_o_dat),
      .sd_we_o(sd_we_o_dat),
      .sd_re_o(sd_re_o_dat),
      .fifo_full(fifo_full[3:4]),
      .fifo_empty(fifo_empty[3:4]),
      .start_dat(st_dat_t),
      .fifo_acces(~fifo_acces_token)
      );


   assign sd_adr_o = fifo_acces_token ? sd_adr_o_cmd : sd_adr_o_dat; 
   assign sd_dat_o = fifo_acces_token ? sd_dat_o_cmd : sd_dat_o_dat;
   assign sd_we_o  = fifo_acces_token ? sd_we_o_cmd : sd_we_o_dat;
   assign sd_re_o  = fifo_acces_token ? sd_re_o_cmd : sd_re_o_dat;

   assign sd_dat_i_dat = sd_dat_i;
   assign sd_dat_i_cmd = sd_dat_i;

   always @(posedge S_AXI_ACLK)
	 begin
	    if (~S_AXI_ARESETN)
	      status_reg<=8'h50;
	    else begin
           status_reg[0] <= fifo_full[1];
           status_reg[1] <= fifo_empty[2];
           status_reg[2] <= fifo_full[3];
           status_reg[3] <= fifo_empty[4];
           status_reg[4] <= status_reg[4] & ~controll_reg[7] | SDIO_NODISK; // NOT Initialized
           status_reg[5] <= SDIO_NODISK; // NODISK
           status_reg[6] <= 1'b1; // PROTECT
        end
     end

   assign S_AXI_BRESP = 2'b00;
   assign S_AXI_RRESP = 2'b00;
   assign S_AXI_AWREADY = (axist == 3'b000)|(axist == 3'b010);
   assign S_AXI_WREADY  = (axist == 3'b000)|(axist == 3'b001);
   assign S_AXI_ARREADY = (axist == 3'b000);
   assign S_AXI_BVALID  = (axist == 3'b011);
   assign S_AXI_RVALID  = (axist == 3'b100);

   always @(posedge S_AXI_ACLK)begin
      if(~S_AXI_ARESETN)begin
         axist<=3'b000;

         command_timeout_reg<=`TIME_OUT_TIME;
         controll_reg<=0;

         wb_adr_i<=0;
         wb_dat_i_storage<=0;

         time_enable<=0;
      end else if(axist==3'b000)begin
         controll_reg[7] <= 1'b0;
         if(S_AXI_AWVALID & S_AXI_WVALID)begin
            axist<=3'b011;
            wb_adr_i<=S_AXI_AWADDR[4:2];
            wb_dat_i_storage<=S_AXI_WDATA[7:0];
            command_timeout_reg<=`TIME_OUT_TIME;
            time_enable<=1;
         end else if(S_AXI_AWVALID)begin
            axist<=3'b001;
            wb_adr_i<=S_AXI_AWADDR[4:2];
         end else if(S_AXI_WVALID)begin
            axist<=3'b010;
            wb_dat_i_storage<=S_AXI_WDATA[7:0];
         end else if(S_AXI_ARVALID)begin
            if((S_AXI_ARADDR[2+2:2]!=`rx_data_fifo)|~fifo_empty[4])begin
               axist<=3'b100;
            end else begin
               axist<=3'b101;
            end
            wb_adr_i<=S_AXI_ARADDR[4:2];
         end
      end else if(axist==3'b001)begin
         if(S_AXI_WVALID)begin
            axist<=3'b011;
            wb_dat_i_storage<=S_AXI_WDATA[7:0];
            command_timeout_reg<=`TIME_OUT_TIME;
            time_enable<=1;
         end
      end else if(axist==3'b010)begin
         if(S_AXI_AWVALID)begin
            axist<=3'b011;
            wb_adr_i<=S_AXI_AWADDR[4:2];
            command_timeout_reg<=`TIME_OUT_TIME;
            time_enable<=1;
         end
      end else if(axist==3'b011)begin
         if(S_AXI_BREADY)begin
            axist<=3'b000;
            case (wb_adr_i)
              `controll : controll_reg <= wb_dat_i_storage;
            endcase
         end
      end else if(axist==3'b100)begin
         if(S_AXI_RREADY&S_AXI_RVALID)begin
            axist<=3'b000;
         end
      end else if(axist==3'b101)begin
         if(~fifo_empty[4])begin
            axist<=3'b100;
         end
      end
      if(time_enable & ~S_AXI_AWVALID & ~S_AXI_WVALID)begin
         if (!status_reg[1])
           time_enable<=0;
         if ((command_timeout_reg!=0) && (time_enable))
           command_timeout_reg<=command_timeout_reg-1;
      end
   end

   always @(*)begin
      case (wb_adr_i)
        `status : wb_dat_o_i = status_reg;
        `timer  : wb_dat_o_i = command_timeout_reg[9:2];
        default : wb_dat_o_i = 8'hxx;
      endcase
   end

endmodule
