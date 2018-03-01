library(tidyverse)
library(ggthemes)

## Importamos los datos y modificamos para que queden en formato conveniente.

nombre_columnas <- c("inst_size", "data_size", "associativity", "block_size", "write_b", "write_alloc",  
                     "inst_accesses", "inst_misses", "inst_miss_rate", "inst_replacements", 
                     "data_accesses", "data_misses", "data_miss_rate", "data_replacements",
                     "demand_fetches", "copies_back")

files_wsc <- c("cc_stats_1_wsc.txt", "spice_stats_1_wsc.txt", "tex_stats_1_wsc.txt")
files_blocksize <- c("cc_stats_2_block.txt", "spice_stats_2_block.txt", "tex_stats_2_block.txt")
files_assoc <- c("cc_stats_3_assoc.txt", "spice_stats_3_assoc.txt", "tex_stats_3_assoc.txt")
files_bandwidth <- c("cc_stats_4_memory_bandwidth.txt", "spice_stats_4_memory_bandwidth.txt", "tex_stats_4_memory_bandwidth.txt")

agrega_df <- function(files_list) {
  dataf <- data_frame(file = files_list) %>% 
    mutate(file_contents = map(files_list, ~read_csv(., col_names = nombre_columnas)))
  df <- unnest(dataf)
  
  df %>%
    mutate(file = case_when(grepl("cc", file)==TRUE ~ "cc",
                            grepl("spice", file)==TRUE ~ "spice",
                            grepl("tex", file)==TRUE ~ "tex")) %>%
    gather(key = data_or_inst, value = miss_rate, inst_miss_rate, data_miss_rate) %>%
    mutate(data_or_inst = case_when(grepl("inst", data_or_inst)==T ~ "inst",
                                    grepl("data", data_or_inst)==T ~ "data")) 
}

wsc <- agrega_df(files_wsc)
blocksize <- agrega_df(files_blocksize)
association <- agrega_df(files_assoc)
bandwidth <- agrega_df(files_bandwidth)

## Localidades únicas en trazas

cc <- read_delim(file = "/trazas/cc.trace",                   # hay que proporcionar el directorio de las trazas
                 delim= " ",
                 col_names = c("data_or_inst", "address"))

spice <- read_delim(file = "/trazas/spice.trace",
                    delim= " ",
                    col_names = c("data_or_inst", "address"))

tex <- read_delim(file = "/trazas/tex.trace",
                  delim= " ",
                  col_names = c("data_or_inst", "address"))

## NÚMERO DE DIRECCIONES TOTALES

### Data
cc %>% filter(data_or_inst == 0 | data_or_inst == 1) %>%  nrow()
spice %>% filter(data_or_inst == 0 | data_or_inst == 1) %>%  nrow()
tex %>% filter(data_or_inst == 0 | data_or_inst == 1) %>%  nrow()

### Instruction
cc %>% filter(data_or_inst == 2) %>%  nrow()
spice %>% filter(data_or_inst == 2) %>%  nrow()
tex %>% filter(data_or_inst == 2) %>%  nrow()


## NÚMERO DE DIRECCIONES ÚNICAS
unidir <- function(df, data_1, data_2){df %>% 
    filter(data_or_inst == data_1 | data_or_inst == data_2) %>% 
    .$address %>% 
    unique() %>% 
    NROW()}

### Data
(unidir(cc, 0, 1))
(unidir(spice, 0, 1))
(unidir(tex, 0, 1))

### Instruction
cc %>% unidir(2,2)
spice %>% unidir(2,2)
tex %>% unidir(2,2)


## WSC

ggplot(data = wsc, aes(x = log(inst_size, 2), y = 1 - miss_rate, color = data_or_inst)) +
  facet_grid(.~file) +
  geom_line() +
  geom_point() +
  scale_x_continuous(trans="log2") + 
  xlab("Tamaño de memoria (2^x)") + ylab(substitute(italic("Hit rate"))) +
  theme_tufte(base_size = 14, base_family = "sans")

## Blocksize

ggplot(data = blocksize, aes(x = log(block_size, 2), y = 1 - miss_rate, color = data_or_inst)) +
  facet_grid(.~file) +
  geom_line() +
  geom_point() +
  scale_x_continuous(trans="log2") + 
  xlab("Block size") + ylab(substitute(italic("Hit rate"))) +
  theme_tufte(base_size = 14, base_family = "sans")

## Associativity

ggplot(data = association, aes(x = log(associativity, 2), y = 1 - miss_rate, color = data_or_inst)) +
  facet_grid(.~file) +
  geom_line() +
  geom_point() +
  scale_x_continuous(trans="log2") + 
  xlab("Asociatividad") + ylab(substitute(italic("Hit rate"))) +
  theme_tufte(base_size = 14, base_family = "sans")

## Bandwidth

grafica_write_through <- function(fileName, cacheSize, blockSize, assoc) {
  bandwidth %>% 
    filter(file == fileName,
           inst_size == cacheSize,
           block_size == blockSize,
           associativity == assoc,
           write_alloc == "WRITE NO ALLOCATE")
} 

grafica_write_back <- function(fileName, cacheSize, blockSize, assoc) {
  bandwidth %>% 
    filter(file == fileName,
           inst_size == cacheSize,
           block_size == blockSize,
           associativity == assoc,
           write_b == "WRITE BACK")
} 

grafica_write_through("cc", 8192, 64, 4)  %>% View()

ggplot(bandwidth, aes(write_b, copies_back, demand_fetches)) +
  geom_col()

library(reshape)
install.packages('reshape')

bandwidthn <- as.data.frame(bandwidth)


bandwidthm <- melt(bandwidthn,id=c('write_b', 'write_alloc', 'block_size', 'associativity', 'data_size', 'inst_size', 'file') )
bandwidthm <- transform(bandwidthm, value = as.numeric(value));
bandwidthm2 <- bandwidthm;
bandwidthm2$write_alloc[bandwidthm2$write_alloc=='WRITE NO ALLOCATE']='WN';
bandwidthm2$write_alloc[bandwidthm2$write_alloc=='WRITE ALLOCATE']='WA';
bandwidthm2 <- bandwidthm2 %>% mutate(write_alloc=paste(write_alloc,'-B',block_size, '-A', associativity));


ggplot( bandwidthm2 %>% filter( variable=='copies_back')
        %>% filter() 
        %>% filter()
        %>% filter(TRUE)
        %>% filter(write_b=="WRITE BACK")
        %>% filter(data_size==8192),aes(x = write_alloc,y = value)) + 
  geom_bar(aes(fill = file),stat = "identity",position = "dodge")+
  theme(axis.text.x=element_text(angle=90,hjust=1,vjust=0.5)) + ggtitle("Copy backs")



ggplot( bandwidthm2 %>% filter( variable=='demand_fetches')
        %>% filter() 
        %>% filter()
        %>% filter(TRUE)
        %>% filter(write_b=="WRITE BACK")
        %>% filter(data_size==8192),aes(x = write_alloc,y = value)) + 
  geom_bar(aes(fill = file),stat = "identity",position = "dodge")+
  theme(axis.text.x=element_text(angle=90,hjust=1,vjust=0.5)) + ggtitle("Demand fetches")







ggplot( bandwidthm2 %>% filter( variable=='demand_fetches')
        %>% filter() 
        %>% filter()
        %>% filter(TRUE)
        %>% filter(write_alloc=="WRITE NO ALLOCATE")
        %>% filter(data_size==8192),aes(x = write_alloc,y = value)) + 
  geom_bar(aes(fill = file),stat = "identity",position = "dodge")+
  theme(axis.text.x=element_text(angle=90,hjust=1,vjust=0.5)) + ggtitle("Fetches")

ggplot( bandwidthm2 %>% filter( variable=='copy_backs')
        %>% filter() 
        %>% filter()
        %>% filter(TRUE)
        %>% filter(write_alloc=="WRITE NO ALLOCATE")
        %>% filter(data_size==8192),aes(x = write_b,y = value)) + 
  geom_bar(aes(fill = file),stat = "identity",position = "dodge")+
  theme(axis.text.x=element_text(angle=90,hjust=1,vjust=0.5)) + ggtitle("Fetches")



type(bandwidthm$value)



grafica_write_back("cc", 8192, 64, 4) %>% View()
