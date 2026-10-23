# URBAN5127｜第五阶段：回归模型诊断与假设检验

步骤 29–34 · Regression Diagnostics & Model Assumption Checking · R / RStudio

我已经查看了你上传的 `HTML_04_regression.html`。这次第五阶段将直接针对你实际完成的 M0、M1、M2 和 M3，不再使用假设的回归结果。

## 一、先核对第四阶段的实际成果

你的 HTML 显示，M0–M2 均使用了同一批 9,052 名成年出行者，而且你已经执行了四档人口密度与家庭车辆数的交互模型 `m3_4`。

第四阶段实际回归结果

| 模型  | R²      | Adjusted R² | Residual SE |
| --- | ------- | ----------- | ----------- |
| M0  | 0.00443 | 0.00333     | 71.20       |
| M1  | 0.00706 | 0.00519     | 71.14       |
| M2  | 0.00905 | 0.00697     | 71.07       |
| M3₄ | 0.00945 | 0.00715     | 71.07       |

Residual SE 单位为英里；M3₄ 使用四档密度，因此不能直接将其与八档密度的 M2 解释为仅增加交互项后的模型。

你已经得到的传统嵌套模型检验结果为：

- M0 → M1： F = 3.4186，p = 0.001184，加入八档人口密度后，传统 OLS 联合检验达到统计显著。
- M1 → M2： F = 9.0857，p = 0.0001143，加入家庭车辆数后，传统 OLS 联合检验也达到统计显著。
- M3₄： 已有完整的模型系数输出，但 HTML 中没有显示 `anova(m2_4, m3_4)` 的实际检验结果，尽管包含了该行代码。因此，目前还不能认定交互项整体显著。

### 这些结果提示了什么？

最需要关注的是：即使到了 M3₄，R² 仍然只有约 0.95%，而残差标准误约为 71 英里。

这并不自动说明研究失败，但表明模型只能解释平均行程距离变异中的很小部分。

更重要的是，你的 M3₄ 残差摘要中：

- 第一四分位数：-16.30 英里；
- 中位数：-10.23 英里；
- 第三四分位数：-2.89 英里；
- 最大残差：约 +2,418.07 英里。

这是一项明显的诊断信号：少数极长行程可能产生非常大的正残差，并对 OLS 估计和常规显著性检验造成影响。

不过，仅凭残差摘要还不能确定存在异方差、错误函数形式或高影响点。接下来必须通过系统诊断来判断。

## 二、第五阶段的六个正式步骤

| 步骤 | 检查内容        | 核心函数                                          | 需要回答的问题           |
| -- | ----------- | --------------------------------------------- | ----------------- |
| 29 | 残差与拟合值      | `residuals()`、`fitted()`、`plot()`             | 模型预测误差是否表现出系统性结构？ |
| 30 | 残差分布与正态性    | `qqnorm()`、`qqline()`、`plot()`                | 残差是否具有严重长尾或偏离正态？  |
| 31 | 异方差         | `bptest()`                                    | 不同预测水平下的误差方差是否相同？ |
| 32 | 函数形式与模型设定   | `resettest()`                                 | 当前线性形式是否可能存在设定不足？ |
| 33 | 多重共线性       | `vif()` / `GVIF`                              | 自变量是否存在明显冗余或不稳定性？ |
| 34 | 离群值、杠杆值与影响点 | `rstudent()`、`hatvalues()`、`cooks.distance()` | 少数观测是否主导估计结果？     |

你已完成 M3，因此每项检查都应区分：

M2： 正式研究主模型，八档人口密度。

M3₄： 交互效应扩展模型，四档人口密度 × 三档车辆拥有量。

我建议以 M2 作为第五阶段完整诊断的主对象，然后对 M3₄ 进行对应的附加诊断。不要只检查 M3₄ 而忽略 M2，因为 M2 保留了原本的八档密度设计，仍然是你的核心研究模型。

# 三、开始操作：建立第五阶段 R Markdown

建议建立：

`R/05_diagnostics.Rmd`

第五阶段直接读取第四阶段的模型，不重新进行数据清洗和描述性统计。

## 0.1 需要安装的 R 包

本阶段使用：

| R 包         | 用途                              |
| ----------- | ------------------------------- |
| `tidyverse` | 整理诊断数据、绘制图表、导出结果                |
| `lmtest`    | Breusch–Pagan 和 Ramsey RESET 检验 |
| `car`       | VIF / GVIF 共线性检验                |

第一次使用时执行：

```
install.packages(c(
  "tidyverse",
  "lmtest",
  "car"
))
```

在 R Markdown 的第一个代码块中写：

```
library(tidyverse)
library(lmtest)
library(car)
```

## 0.2 读取 M0、M1、M2

你第四阶段的 HTML 使用了自动判断项目根目录的方式，我们继续保持相同的结构，避免 RStudio Knit 时找不到文件。

```
# ==================================================
# Stage 5: Model Diagnostics
# ==================================================

library(tidyverse)
library(lmtest)
library(car)

# Locate project root
if (file.exists("../data/processed/model_data.rds")) {
  project_root <- ".."
} else if (file.exists("data/processed/model_data.rds")) {
  project_root <- "."
} else {
  stop("Cannot find model_data.rds")
}

# Define folders
processed_dir <- file.path(
  project_root, "data", "processed"
)

tables_dir <- file.path(project_root, "tables")
figures_dir <- file.path(project_root, "figures")

dir.create(tables_dir, showWarnings = FALSE)
dir.create(figures_dir, showWarnings = FALSE)

# Read Stage 4 model sample
model_data <- readRDS(
  file.path(processed_dir, "model_data.rds")
)

# Read the saved model bundle
model_bundle <- readRDS(
  file.path(processed_dir, "regression_models.rds")
)

m0 <- model_bundle$M0
m1 <- model_bundle$M1
m2 <- model_bundle$M2

# Check models
nobs(m0)
nobs(m1)
nobs(m2)

summary(m2)
```

预期三者的观测数均为：

`9052`

### 关于 M3：你的第四阶段有一处需要补充

我检查了 HTML 最后的模型保存代码。

你虽然已经成功运行 `m2_4` 和 `m3_4`，但保存的 `regression_models.rds` 中只有 M0、M1 和 M2，没有保存 M3。

因此，如果你关闭 RStudio 后重新打开第五阶段，不能直接使用：

```
summary(m3_4)
```

除非之前重新执行了 M3 的建模代码。

为了保证第五阶段可以独立运行，建议加入下面的代码。

## 0.3 重新载入 M3₄ 的模型设定

这里不是重新分析数据，而是使用已经保存的 `model_data`，按第四阶段完全相同的公式重建 M3。

```
# ==================================================
# Reconstruct M3 using Stage 4 specifications
# ==================================================

# Create density_4 if not already present
if (!"density_4" %in% names(model_data)) {

  model_data <- model_data %>%
    mutate(
      density_4 = case_when(
        as.character(density_band) %in%
          c("01", "02") ~ "D1",
        as.character(density_band) %in%
          c("03", "04") ~ "D2",
        as.character(density_band) %in%
          c("05", "06") ~ "D3",
        as.character(density_band) %in%
          c("07", "08") ~ "D4",
        TRUE ~ NA_character_
      )
    )
}

# Ensure correct order
model_data <- model_data %>%
  mutate(
    density_4 = factor(
      as.character(density_4),
      levels = c("D1", "D2", "D3", "D4")
    ),

    car_group = relevel(
      factor(car_group),
      ref = "1"
    ),

    worker = relevel(
      factor(worker),
      ref = "not_worker"
    )
  )

stopifnot(!anyNA(model_data$density_4))

# Four-band model without interaction
m2_4 <- lm(
  mean_trip_miles ~
    age_c +
    I(age_c^2) +
    worker +
    hh_size +
    day_of_week +
    density_4 +
    car_group,
  data = model_data
)

# Four-band model with interaction
m3_4 <- update(
  m2_4,
  . ~ . + density_4:car_group
)

# Check against Stage 4
nobs(m3_4)
summary(m3_4)

# Previously missing joint test
anova(m2_4, m3_4)
```

### 核对重建结果

根据你第四阶段的 HTML，M3₄ 重建后应满足：

| 指标                          | 预期         |
| --------------------------- | ---------- |
| N                           | 9,052      |
| Residual SE                 | 约 71.07    |
| R²                          | 约 0.009453 |
| Adjusted R²                 | 约 0.007149 |
| Residual degrees of freedom | 9,030      |

如果明显不一致，先核查因子参考类别、`density_4` 分组和样本是否相同。

随后保存：

```
saveRDS(
  list(
    M2_4 = m2_4,
    M3_4 = m3_4
  ),
  file.path(
    processed_dir,
    "interaction_models.rds"
  )
)
```

这样第五阶段以后也能单独读取 M3，不必每次从第四阶段重新运行全部代码。

# 四、步骤 29：残差与拟合值诊断

Residuals vs Fitted Values

## 29.1 什么是残差？

对于第 \\(i\\) 个受访者：

\\[ e_i=Y_i-\hat{Y}\_i \\]

其中：

- \\(Y_i\\)：实际观测到的个人平均行程距离；
- \\(\hat{Y}\_i\\)：模型预测的平均行程距离；
- \\(e_i\\)：实际值与预测值的差，也就是残差。

例如某人的实际平均行程距离是 20 英里，模型预测为 12 英里，那么：

\\[ e_i=20-12=8 \\]

残差是 +8 英里，说明模型对这名受访者的距离预测偏低。

如果残差是 -8 英里，说明模型的预测值比实际值高 8 英里。

## 29.2 为什么要绘制 Residuals vs Fitted？

经典线性模型的重要设定之一是：

\\[ E(\varepsilon_i\mid X_i)=0 \\]

即在解释变量给定的条件下，误差的条件期望为零。

如果模型遗漏了某种重要的非线性结构，残差图可能显示系统性规律。

但有一点必须理解：

残差图只能提供模型设定问题的线索，不能证明零条件均值假设成立，也不能排除遗漏变量偏误。

## 29.3 三种典型残差形态

Residuals vs Fitted：示意图

较随机

漏斗形

弯曲结构

示意：左侧缺少明显系统结构；中间可能存在异方差；右侧可能表示模型未充分捕捉非线性。实际数据图形会更复杂。

## 29.4 执行 R 代码

首先检查主模型 M2：

```
# ==================================================
# Step 29: Residuals vs Fitted
# ==================================================

# Main eight-density model
plot(
  m2,
  which = 1
)

# Interaction model
plot(
  m3_4,
  which = 1
)
```

`plot(lm_object, which = 1)` 是 R 自带的模型诊断图，不需要安装额外绘图包。

你要重点观察红色平滑线是否明显偏离水平零线，以及残差是否随着拟合值增加而系统性扩大。

## 29.5 进一步构建残差数据

为了便于后续导出、定位异常观测，我们可以创建独立的数据框。

```
# ==================================================
# Build diagnostic data for M2
# ==================================================

stopifnot(
  nobs(m2) == nrow(model_data)
)

# Confirm response values remain in the same order
stopifnot(
  isTRUE(all.equal(
    as.numeric(model_data$mean_trip_miles),
    as.numeric(fitted(m2) + residuals(m2)),
    tolerance = 1e-8
  ))
)

diag_m2 <- model_data %>%
  mutate(
    row_id = row_number(),
    fitted_value = as.numeric(fitted(m2)),
    residual = as.numeric(residuals(m2))
  )

summary(diag_m2$residual)
```

这里先检查样本与排序，是为了确保第 \\(i\\) 个残差确实对应 `model_data` 的第 \\(i\\) 名受访者。

随后可使用 `ggplot2`：

```
p_residuals <- ggplot(
  diag_m2,
  aes(
    x = fitted_value,
    y = residual
  )
) +
  geom_point(alpha = 0.15, size = 0.7) +
  geom_hline(
    yintercept = 0,
    linetype = "dashed"
  ) +
  geom_smooth(
    method = "loess",
    se = FALSE,
    colour = "red"
  ) +
  labs(
    title = "Residuals vs Fitted Values - M2",
    x = "Fitted mean trip distance (miles)",
    y = "Residual (miles)"
  ) +
  theme_minimal()

print(p_residuals)
```

### 如何解读你的 NHTS 模型？

由于第四阶段 M3₄ 已经出现超过 2,400 英里的最大正残差，我预计完整的残差图可能受到极端观测的明显影响。

这是依据已有残差摘要提出的待验证判断，并不是已经完成的 M2 残差检验结果。

如果发现明显长尾或漏斗形，应该记录下来，后续结合异方差检验、影响点分析和距离变换敏感性分析处理，而不是立刻删除这些数据。

# 五、步骤 30：残差正态性与 Q-Q 图

Normal Q-Q Plot

## 30.1 为什么要检查残差正态性？

在线性回归中，残差正态性是传统小样本 t 检验、F 检验所使用的经典正态误差假设之一。

标准的正态误差假设可写为：

\\[ \varepsilon_i\mid X_i\sim N(0,\sigma^2) \\]

但有一个非常重要的区别：

OLS 不要求原始因变量必须正态分布。

OLS 系数的计算也不需要残差完全正态。正态性主要涉及某些传统有限样本推断的精确性。

对于你的 9,052 个观测，样本量较大有助于某些渐近推断，但如果极端观测具有很强影响，不能仅凭“大样本”就认为传统标准误和显著性结论一定可靠。

## 30.2 Q-Q 图的原理

Q-Q 图比较的是：

- 纵轴：样本残差的分位数；
- 横轴：理论正态分布的相应分位数。

如果残差大致符合正态分布，点通常靠近参考直线。

Q-Q 图示意

大致接近正态

右尾明显偏离

示意图，不代表真实 NHTS 诊断结果。

## 30.3 R 代码

```
# ==================================================
# Step 30: Normal Q-Q plots
# ==================================================

# M2
plot(m2, which = 2)

# M3 interaction model
plot(m3_4, which = 2)
```

也可以用基础函数：

```
qqnorm(
  residuals(m2),
  main = "Normal Q-Q Plot - M2"
)

qqline(
  residuals(m2),
  col = "red",
  lwd = 2
)
```

### 观察重点

对于你的数据，应重点查看：

1. 左尾是否系统偏离参考线。
2. 右尾是否明显向上偏离。
3. 是否只有几个特别极端的残差造成偏离。
4. M2 与 M3₄ 的残差形态是否相似。

### 是否需要 Shapiro–Wilk 检验？

不建议在当前研究中把它作为主要依据。

一方面，R 的 `shapiro.test()` 对样本数量有限制，不能直接用于你的全部 9,052 个残差。

另一方面，即使抽样检验，在较大样本下，也容易识别统计上可检测、但实际意义较小的正态性偏离。

对于你的研究，Q-Q 图、残差分布和影响点分析更有实际价值。

# 六、步骤 31：异方差检验

Heteroskedasticity — Breusch–Pagan Test

## 31.1 什么是异方差？

经典同方差假设为：

\\[ Var(\varepsilon_i\mid X_i)=\sigma^2 \\]

意思是：在不同解释变量取值下，误差的条件方差相同。

如果误差的方差随解释变量变化，就可能存在异方差。

例如，短距离出行者的模型预测误差可能相对较小，而包含极长行程的群体，其误差波动可能非常大。

异方差不一定使 OLS 系数本身有偏，但会影响普通 OLS 方差估计，进而影响传统 t 检验、F 检验和置信区间的可靠性。

## 31.2 Breusch–Pagan 检验假设

\\[ H_0:\text{同方差} \\]

\\[ H_1:\text{存在被检验形式的异方差} \\]

解释时：

| 检验结果     | 正确理解                   |
| -------- | ---------------------- |
| p < 0.05 | 拒绝同方差零假设，有异方差证据        |
| p ≥ 0.05 | 未能拒绝同方差零假设，但不能证明不存在异方差 |

这里 0.05 是常用的事先设定显著性水平，不是模型是否可用的绝对分界。

## 31.3 对 M2 进行 BP 检验

```
# ==================================================
# Step 31: Breusch-Pagan tests
# ==================================================

bp_m2 <- lmtest::bptest(m2)

print(bp_m2)
```

输出通常类似：

```
studentized Breusch-Pagan test

BP = ...
df = ...
p-value = ...
```

`BP` 是检验统计量，`df` 是辅助方差回归的自由度。

`bptest()` 默认采用学生化版本的 Breusch–Pagan 检验。

## 31.4 对 M3₄ 进行 BP 检验

```
bp_m3 <- lmtest::bptest(m3_4)

print(bp_m3)
```

为什么两个模型都要检验？

因为加入交互项后，模型对条件均值的拟合可能发生变化，其残差结构也可能发生变化。

但如果 M3₄ 的 BP 检验不显著，而 M2 的 BP 检验显著，不能仅因此认定 M3₄ 是更好的研究模型。还需要考虑交互项的理论依据、样本支持和模型稳定性。

## 31.5 检测到异方差后怎么办？

不要直接删除观测值，也不需要仅因为 BP 检验显著就放弃 OLS。

后续第六阶段可以比较：

- 普通 OLS 标准误；
- HC3 异方差稳健标准误；
- 按 `HOUSEID` 聚类的稳健标准误。

这里先完成诊断并记录结果。

对于你的数据，家庭内部相关是另一个独立问题，普通 BP 检验没有同时解决调查抽样结构的问题。

# 七、步骤 32：函数形式与 Ramsey RESET 检验

Model Specification — Ramsey RESET

## 32.1 为什么需要这项检验？

即使我们加入了年龄平方项，模型也不一定已经正确描述了所有变量与行程距离之间的条件关系。

例如，年龄的实际关系可能并非简单二次曲线；家庭规模可能存在非线性；某些变量之间可能存在尚未建模的交互。

Ramsey RESET 是一种模型设定的综合性检验。

它通过考察拟合值的高次幂能否提供额外解释，检查现有模型是否存在某些形式的设定不足。

## 32.2 检验假设

\\[ H_0:\text{额外 RESET 项的系数同时为 0} \\]

如果拒绝零假设，提示现有模型可能存在函数形式设定问题。

但必须强调：

RESET 检验不能告诉你具体遗漏了哪个变量，也不能证明某个变量具有因果作用。

## 32.3 执行代码

```
# ==================================================
# Step 32: Ramsey RESET tests
# ==================================================

reset_m2 <- lmtest::resettest(
  m2,
  power = 2:3,
  type = "fitted"
)

print(reset_m2)
```

参数讲解：

`m2`：待诊断的回归模型。

`power = 2:3`：在辅助检验中考虑拟合值的平方和立方项。

`type = "fitted"`：使用原模型拟合值构造这些额外项。

### M3₄ 的 RESET

```
reset_m3 <- lmtest::resettest(
  m3_4,
  power = 2:3,
  type = "fitted"
)

print(reset_m3)
```

## 32.4 如何解释？

| 结果            | 可得出的结论                  |
| ------------- | ----------------------- |
| p < 0.05      | 存在模型设定不足的统计证据           |
| p ≥ 0.05      | 未检出 RESET 所针对的特定设定不足    |
| M2 显著、M3₄ 不显著 | 交互可能改变了部分拟合结构，但仍需其他证据支持 |

如果 RESET 显著，下一步应结合残差图与理论分析，考虑非线性、遗漏变量或其他合理的模型形式。

但不要无限增加解释变量，只为了让 RESET 的 P 值超过 0.05。

对于你的课程，能合理解释模型限制比机械追求所有假设检验“不显著”更重要。

# 八、步骤 33：多重共线性检验

Multicollinearity — VIF / GVIF

## 33.1 什么是多重共线性？

多重共线性指不同解释变量之间存在较强的线性关系。

例如，你的研究中可能存在：

- 人口密度与家庭车辆拥有量之间的相关关系；
- 年龄与年龄平方项之间的相关关系；
- M3₄ 中密度主效应、车辆主效应和二者交互项之间的相关关系。

严重的多重共线性不会必然破坏模型的整体拟合，但可能使某些回归系数的方差增大，表现为标准误变大、置信区间变宽、系数不稳定。

## 33.2 VIF 的含义

VIF 是 Variance Inflation Factor（方差膨胀因子）。

对于一个具有单一自由度的解释变量：

\\[ VIF_j=\frac{1}{1-R_j^2} \\]

这里的 \\(R_j^2\\) 是把该解释变量作为因变量、用其他解释变量解释它时得到的 \\(R^2\\)，不是主回归模型的 \\(R^2\\)。

例如：

- VIF = 1：不存在由其他解释变量造成的线性方差膨胀；
- VIF = 5：对应系数的方差膨胀为无共线性基准情形的 5 倍；
- VIF = 10：提示可能存在严重的共线性问题。

但 5 和 10 都只是经验性参考，不能作为自动删除变量的规则。

## 33.3 对 M2 执行 VIF

```
# ==================================================
# Step 33: Multicollinearity diagnostics
# ==================================================

vif_m2 <- car::vif(m2)

print(vif_m2)
```

因为你的模型包含人口密度、工作状态、汽车拥有量和调查星期等因子变量，输出可能包含：

```
GVIF
Df
GVIF^(1/(2*Df))
```

### GVIF 与 VIF 有什么区别？

普通 VIF 主要针对一个自由度的系数。

但 `density_band` 有八个类别，实际占用七个模型自由度，因此需要使用 Generalised Variance Inflation Factor（GVIF）评价相应的系数集合。

对于多自由度项，常用调整指标为：

\\[ GVIF^{1/(2Df)} \\]

它使不同自由度的项更容易进行粗略比较。

不要把原始 GVIF 直接与普通 VIF 的 5 或 10 阈值进行简单比较。 调整后的 GVIF 也不能机械地使用与普通 VIF 完全相同的阈值，因为其尺度和含义不同。

### 检查结果时应重点关注

| 变量             | 重点                      |
| -------------- | ----------------------- |
| `age_c`        | 与 `I(age_c^2)` 是否存在较强关联 |
| `I(age_c^2)`   | 年龄平方项是否造成明显方差膨胀         |
| `density_band` | 与车辆拥有量等解释变量之间是否有较强冗余    |
| `car_group`    | 车辆分类是否被其他变量高度预测         |
| `day_of_week`  | 多类别虚拟变量整体是否存在共线性问题      |

## 33.4 M3₄ 应怎样检查共线性？

M3₄ 包含交互项，不能机械地把每个交互项当作完全独立的解释变量进行解读。

首先检查模型设计矩阵是否满秩。

```
# ==================================================
# Step 33.4: M3 design-matrix checks
# ==================================================

# Obtain the model matrix
X_m3 <- model.matrix(m3_4)

# Number of model columns
ncol(X_m3)

# Matrix rank
qr(X_m3)$rank

# Identify perfectly aliased coefficients
alias(m3_4)
```

### 如何理解？

如果：

```
qr(X_m3)$rank == ncol(X_m3)
```

说明设计矩阵满列秩，没有完全线性依赖。

如果矩阵不满秩，可能存在完全共线性，部分系数无法独立估计。

`alias()` 可以帮助确定哪些系数因完全线性关系而无法独立识别。

接下来可以尝试：

```
car::vif(m3_4, type = "predictor")
```

对于包含交互项的模型，`type = "predictor"` 按涉及该预测变量的项进行分组，和普通的逐项 VIF 含义不同。某些预测变量组合可能覆盖整个模型，因此这项输出只能辅助判断，不宜直接进行阈值比较。

### 你这个 M3 最需要注意的地方

你的实际交叉样本数据显示：

| 交叉类别    | 人数  | 家庭数 |
| ------- | --- | --- |
| D1 × 无车 | 22  | 20  |
| D2 × 无车 | 28  | 26  |
| D3 × 无车 | 116 | 96  |
| D4 × 无车 | 120 | 104 |

这说明合并四档后，前两个低密度无车组仍然比较小。

而你的 M3₄ 结果中：

- `density_4D2:car_group0` 的标准误约为 20.83；
- `density_4D3:car_group0` 的标准误约为 17.04；
- `density_4D4:car_group0` 的标准误约为 17.30。

这些相对较大的标准误说明相应交互系数估计具有明显不确定性，但仅凭标准误较大，不能认定其原因一定是多重共线性。稀疏样本、残差方差较大和变量分布都可能造成这种现象。

因此，对 M3₄ 的综合判断应包括：

1. 是否存在完全共线性；
2. 是否有稀疏交叉类别；
3. 交互项的联合检验是否显著；
4. 系数与预测差异的不确定性有多大；
5. 是否具有值得在论文中讨论的实际交通行为意义。

# 九、步骤 34：离群值、杠杆值与影响点

Outliers, Leverage & Influential Observations

这是针对你目前 NHTS 模型尤其重要的一步。

你的因变量具有很长的右尾，M3₄ 的最大残差达到约 +2,418 英里。

因此，我们需要区分三个不同概念。

## 34.1 三种需要区分的观测

| 类型    | 英文                      | 含义                   |
| ----- | ----------------------- | -------------------- |
| 残差离群点 | Residual outlier        | 实际结果与模型预测相差很大的观测     |
| 高杠杆点  | High leverage           | 自变量组合在样本中比较特殊的观测     |
| 高影响点  | Influential observation | 删除该观测可能明显改变模型估计结果的观测 |

不是所有离群点都是高影响点，也不是所有高杠杆点都具有很大的残差。

三个概念的关系（示意）

示意：高影响点往往综合了特殊的解释变量取值与明显的预测误差，具体需要使用影响度量判断。

## 34.2 Studentized Residuals：学生化残差

原始残差的大小以英里计量，不同模型之间可能不方便比较。

学生化残差将其相对于估计的误差尺度进行标准化，并在计算外部学生化残差时考虑删除该观测后的误差估计。

在 R 中可以使用：

```
# ==================================================
# Step 34.2: Studentized residuals
# ==================================================

studentized_m2 <- rstudent(m2)

summary(studentized_m2)

# Number of observations with absolute value > 3
sum(abs(studentized_m2) > 3)
```

常见的探索性参考为：

\\[ |r_i|>3 \\]

它表示某个观测的残差相对较大，值得进一步检查。

但这不是自动删除规则，而且在数千次观察同时检查时，即使不存在数据错误，也可能有部分观测超过该阈值。

## 34.3 Leverage：杠杆值

杠杆值衡量一个观测在解释变量空间中有多特殊。

R 函数：

```
# ==================================================
# Step 34.3: Leverage
# ==================================================

leverage_m2 <- hatvalues(m2)

summary(leverage_m2)

# Number of estimated model coefficients
p <- m2$rank

# Number of observations
n <- nobs(m2)

# Descriptive leverage threshold
leverage_threshold <- 2 * p / n

leverage_threshold

sum(leverage_m2 > leverage_threshold)
```

这里：

- `p` 是模型设计矩阵的秩；
- `n` 是样本数量；
- `2p/n` 是一种常见的启发式筛查阈值。

它不是统计显著性检验，也不能说明超过阈值的受访者一定有问题。

对于本研究，较少见的人口密度与汽车拥有结构，可能对应较特殊的解释变量组合。

## 34.4 Cook's Distance：影响度量

Cook's Distance 综合考虑某个观测对模型系数估计的影响。

```
# ==================================================
# Step 34.4: Cook's Distance
# ==================================================

cook_m2 <- cooks.distance(m2)

summary(cook_m2)

# Exploratory threshold
cook_threshold <- 4 / nobs(m2)

cook_threshold

# Count observations above threshold
sum(cook_m2 > cook_threshold)

# Plot Cook's Distance
plot(
  m2,
  which = 4
)
```

对于 9,052 个观测，`4/n` 大约为 0.000442。

这个数值很小，因此可能有较多观测被标记。

不要只因为 Cook's D 超过 `4/n` 就批量删除记录。 该阈值主要用于寻找值得检查的观测。

## 34.5 提取最有影响的 15 条观测

建议构建一张内部诊断表。

```
# ==================================================
# Step 34.5: Identify influential observations
# ==================================================

diag_m2 <- diag_m2 %>%
  mutate(
    studentized_residual = as.numeric(rstudent(m2)),
    leverage = as.numeric(hatvalues(m2)),
    cooks_d = as.numeric(cooks.distance(m2))
  )

# Rank observations by Cook's Distance
top_influential <- diag_m2 %>%
  arrange(desc(cooks_d)) %>%
  select(
    row_id,
    mean_trip_miles,
    density_band,
    car_group,
    fitted_value,
    residual,
    studentized_residual,
    leverage,
    cooks_d
  ) %>%
  slice_head(n = 15)

print(top_influential)
```

### 这张表需要检查什么？

重点查看：

- 是否有特别大的平均行程距离；
- 这些观测主要落在哪些人口密度组；
- 是否来自特定车辆拥有类别；
- 模型预测值是否明显偏低；
- 是否存在单个或少量观测具有异常大的 Cook's D。

这一步只是在识别影响点，不是立即修改模型。

### 为什么不能直接删除？

例如某名受访者确实有合法的极长行程，那么它可能属于真实交通行为，而不是数据录入错误。

如果任意删除这类记录，你实际上可能改变研究对象。

更合理的做法是：

第五阶段识别并报告高影响观测；第六阶段再按照事先明确的规则开展极端值敏感性分析。

这样可以观察结论是否受到少数极端受访者影响，同时保留研究的透明性。

## 34.6 对 M3₄ 进行同样的检查

```
# ==================================================
# Step 34.6: M3 influence diagnostics
# ==================================================

summary(rstudent(m3_4))

summary(hatvalues(m3_4))

summary(cooks.distance(m3_4))

# Standard diagnostic plots
plot(m3_4, which = 4)
plot(m3_4, which = 5)

# Compare Cook's D distributions
summary(cooks.distance(m2))
summary(cooks.distance(m3_4))
```

这里不能仅凭 M3₄ 的 Cook's D 分布略有改善就选择 M3₄。

M3₄ 多了交互项，而且使用了不同的人口密度分组，应结合研究目的与其整体不确定性进行评价。

# 十、如何整理第五阶段的诊断结果？

完成上述六个步骤后，不建议把 R Console 里几十页的输出全部放进正式论文。

更合理的是将结果整理成一张模型诊断汇总表，正文重点讨论其中有研究意义的问题。

## 1. 生成统计检验结果表

以下代码使用前面已经得到的检验对象。

```
# ==================================================
# Stage 5: Diagnostic summary table
# ==================================================

diagnostic_table <- tibble(
  Model = c("M2", "M3_4"),

  N = c(
    nobs(m2),
    nobs(m3_4)
  ),

  BP_statistic = c(
    unname(bp_m2$statistic),
    unname(bp_m3$statistic)
  ),

  BP_p_value = c(
    bp_m2$p.value,
    bp_m3$p.value
  ),

  RESET_statistic = c(
    unname(reset_m2$statistic),
    unname(reset_m3$statistic)
  ),

  RESET_p_value = c(
    reset_m2$p.value,
    reset_m3$p.value
  ),

  Max_Cooks_D = c(
    max(cooks.distance(m2)),
    max(cooks.distance(m3_4))
  ),

  N_Cooks_above_4n = c(
    sum(cooks.distance(m2) > 4 / nobs(m2)),
    sum(cooks.distance(m3_4) > 4 / nobs(m3_4))
  )
)

print(diagnostic_table)
```

表格将包含以下信息：

| 检验              | M2    | M3₄   |
| --------------- | ----- | ----- |
| 观测数量            | 9,052 | 9,052 |
| BP statistic    | 运行后填写 | 运行后填写 |
| BP p-value      | 运行后填写 | 运行后填写 |
| RESET statistic | 运行后填写 | 运行后填写 |
| RESET p-value   | 运行后填写 | 运行后填写 |
| 最大 Cook's D     | 运行后填写 | 运行后填写 |
| Cook's D 超阈值人数  | 运行后填写 | 运行后填写 |

VIF / GVIF 更适合单独保存，因为它们属于各解释变量的诊断，而不是整个模型只有一个的统计量。

## 2. 保存模型诊断图

可以为 M2 保存四张主要诊断图。

```
# ==================================================
# Save M2 diagnostic plots
# ==================================================

for (i in c(1, 2, 4, 5)) {

  png(
    filename = file.path(
      figures_dir,
      paste0("M2_diagnostic_", i, ".png")
    ),
    width = 1800,
    height = 1300,
    res = 220
  )

  plot(m2, which = i)

  dev.off()
}

# Save the residual scatterplot
ggsave(
  filename = file.path(
    figures_dir,
    "M2_residuals_vs_fitted_ggplot.png"
  ),
  plot = p_residuals,
  width = 8,
  height = 5,
  dpi = 300
)
```

其中：

| 图 | 文件                    | 诊断作用                  |
| - | --------------------- | --------------------- |
| 1 | `M2_diagnostic_1.png` | Residuals vs Fitted   |
| 2 | `M2_diagnostic_2.png` | Normal Q-Q            |
| 4 | `M2_diagnostic_4.png` | Cook's Distance       |
| 5 | `M2_diagnostic_5.png` | Residuals vs Leverage |

同样可以把循环中的 `m2` 改为 `m3_4`，保存 M3₄ 的诊断图，注意避免覆盖原文件。

## 3. 保存表格和诊断数据

```
# ==================================================
# Export Stage 5 results
# ==================================================

write_csv(
  diagnostic_table,
  file.path(
    tables_dir,
    "14_diagnostic_summary.csv"
  )
)

write_csv(
  top_influential,
  file.path(
    tables_dir,
    "15_top_influential_observations.csv"
  )
)

write_csv(
  diag_m2,
  file.path(
    tables_dir,
    "16_M2_diagnostic_observations.csv"
  )
)

saveRDS(
  list(
    BP_M2 = bp_m2,
    BP_M3 = bp_m3,
    RESET_M2 = reset_m2,
    RESET_M3 = reset_m3,
    VIF_M2 = vif_m2
  ),
  file.path(
    processed_dir,
    "diagnostic_results.rds"
  )
)
```

这样，第六阶段能够直接使用第五阶段的诊断结果，而不用反复执行所有操作。

# 十一、诊断出现问题后，到底应该怎么办？

这是本阶段最重要的研究方法问题。

做这些统计检验不是为了获得一张全部显示“通过”的清单，而是判断模型在哪些方面受到限制，以及后续哪些修正有必要。

| 诊断发现             | 可能意味着什么              | 下一步建议                      |
| ---------------- | -------------------- | -------------------------- |
| 残差图有明显曲线         | 条件均值模型可能存在形式设定不足     | 结合 RESET 与理论考虑非线性或遗漏项      |
| Q-Q 图右尾明显偏离      | 残差分布具有长尾，传统正态误差假设不合理 | 研究极端值影响，开展距离变换敏感性分析        |
| BP 检验显著          | 存在异方差证据              | 比较 HC3 与家庭聚类稳健推断           |
| RESET 检验显著       | 模型可能存在函数形式设定不足       | 对变量形式和遗漏机制进行理论讨论           |
| VIF / GVIF 较高    | 部分系数估计可能存在较强共线性      | 检查变量编码、冗余和模型结构             |
| Cook's D 有特别大的观测 | 少数记录可能明显影响 OLS 系数    | 检查数据有效性，并在下一阶段做影响敏感性分析     |
| M3₄ 交互估计不稳定      | 某些密度 × 车辆组的信息不足      | 保留 M2 作为主要模型，将 M3₄ 作为探索性结果 |

特别强调：如果发现模型违反某些经典 OLS 假设，不需要为了使所有检验“通过”而反复删除数据、修改分类或增加变量。

一篇高质量的定量研究论文，可以明确发现并承认模型设定的不足，同时展示合理的稳健性检查。

对于你的研究，这是比简单获得更多显著系数更有学术价值的工作。

# 十二、第五阶段如何写入最终论文？

第五阶段对应正式论文中的：

Model Diagnostics and Assumptions

建议最终正文只保留两至三段重点分析，详细诊断图可以根据篇幅选择展示。

### 第一段：残差分布与误差方差

说明残差图、Q-Q 图、BP 检验是否支持传统的同方差和正态误差设定。

如果异方差显著，应说明传统 OLS 标准误可能不可靠，并将在稳健性分析中报告适当修正的统计推断。

### 第二段：函数形式与多重共线性

说明 RESET 结果、VIF/GVIF 指标以及年龄平方项和分类变量的处理依据。

如果发现函数形式问题，应解释这种问题可能意味着什么，而不是仅报告一个检验 P 值。

### 第三段：极端值与研究局限

结合影响点分析，讨论少数长途出行是否可能影响 OLS 估计。

同时需要指出，残差诊断无法解决居住自选择、未观测出行偏好、遗漏交通供给特征等潜在的内生性问题。

# 十三、第五阶段完成后的验收清单

第五阶段完成进度

0/13

重新读取 M0–M2，并成功重建与保存 M3₄

补充运行 anova(m2_4, m3_4)，确认交互项的联合检验

步骤29：完成 M2、M3₄ 的 Residuals vs Fitted 图

步骤30：完成 M2、M3₄ 的 Q-Q 图

步骤31：完成两个模型的 Breusch–Pagan 检验

步骤32：完成两个模型的 Ramsey RESET 检验

步骤33：完成 M2 的 VIF/GVIF 分析

步骤33：检查 M3₄ 的矩阵秩、完全共线性及稀疏组

步骤34：计算学生化残差、杠杆值和 Cook's Distance

步骤34：提取并检查最有影响的15条观测

生成模型诊断统计汇总表

导出全部诊断图、CSV 和 RDS 结果

能够解释每项检验的含义和模型的主要局限

&#x20;复制第五阶段验收清单

## 十四、结合你已经完成的第四阶段，我的建议

你目前最值得深入研究的不是继续添加更多解释变量，而是为什么包含人口密度、年龄、工作状态、家庭规模、星期和汽车拥有量以后，模型依然只解释了不到 1% 的距离变异。

这不是说人口密度不重要，而是说明个体的单日平均行程距离存在很强的异质性，现有模型捕捉到的解释信息有限。

你现在尤其应该关注三个问题：

第一，极端长距离出行对 OLS 有多大影响？ M3₄ 的最大正残差超过 2,400 英里，因此 Q-Q 图和 Cook's Distance 尤其值得检查。

第二，当前常规显著性检验是否可靠？ 尽管 M0→M1 和 M1→M2 的传统 F 检验显著，仍需要结合异方差、家庭聚类及抽样设计重新审视其不确定性。

第三，M3₄ 交互是否真正提供额外解释？ 你的 M3₄ 已经运行，但还需要得到 `anova(m2_4, m3_4)` 的实际结果。部分无车交叉组样本较少，交互系数也存在较大不确定性。

第五阶段建议按照 29 → 30 → 31 → 32 → 33 → 34 的顺序操作，每一步保存输出，再整理诊断汇总表。

完成后，下一阶段才根据真实诊断结果实施第 35–38 步：异方差与家庭聚类稳健标准误、原始距离和对数距离模型比较、人口密度统计尺度敏感性、极端值及调查权重敏感性分析。

这样可以让最终论文形成一条完整、可追溯的证据链：

描述性现象 → 多元回归关系 → 模型假设诊断 → 稳健性验证 → 批判性研究结论。